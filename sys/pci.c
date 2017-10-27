#include<sys/defs.h>
#include<sys/kprintf.h>
#include<sys/asmio.h>
#include<sys/ahci.h>


#define    SATA_SIG_ATA    0x00000101    // SATA drive
#define    SATA_SIG_ATAPI    0xEB140101    // SATAPI drive
#define    SATA_SIG_SEMB    0xC33C0101    // Enclosure management bridge
#define    SATA_SIG_PM    0x96690101    // Port multiplier
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

#define HBA_PxCMD_ICC  1U << 28
#define HBA_PxCMD_POD  1U << 2
#define HBA_PxCMD_SUD  1U << 1
#define HBA_Px_SCTL_IPM 0x00000300
#define HBA_Px_SCTL_DET 0x00000001
#define HBA_CAP_SSS 1U<<27


#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1
#define AHCI_BASE       0xA6000      // 4M
#define PORT_BASE       0x90000

typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef uint16_t WORD;

uint16_t pciReadRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunc = (uint32_t) func;
    uint16_t tmp = 0;

    address = (uint32_t)((lbus << 16) | (lslot << 11) |
                         (lfunc << 8) | (offset & 0xfc) | ((uint32_t) 0x80000000));

    outl(0xCF8, address);
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}

// Stop command engine
void stop_cmd(hba_port_t *port) {
    // Clear ST (bit0)
    port->cmd &= ~HBA_PxCMD_ST;

    // Wait until FR (bit14), CR (bit15) are cleared
    while (1) {
        if (port->cmd & HBA_PxCMD_FR)
            continue;
        if (port->cmd & HBA_PxCMD_CR)
            continue;
        break;
    }

    // Clear FRE (bit4)
    port->cmd &= ~HBA_PxCMD_FRE;
}


// Start command engine
void start_cmd(hba_port_t *port) {
    while (port->cmd & HBA_PxCMD_CR);
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}


static int check_type(hba_port_t *port) {
   	
	//Compare port signature with known port signatures of drive
    switch (port->sig) {
        case SATA_SIG_ATAPI:
            return 2;
        case SATA_SIG_SEMB:
            return 3;
        case SATA_SIG_PM:
            return 4;
        case SATA_SIG_ATA:
            return 1;
        default :
            return 0;
    }
}


void memset(void *ptr, int character, uint16_t length) {
    unsigned char *p = ptr;
    while (length > 0) {
        *p = character;
        p++;
        length--;
    }

}


void port_rebase(hba_port_t *port, int portno) {

    port->clb = (uint64_t)(PORT_BASE + (portno << 12));
    memset((void *) (port->clb), 0, 1024);

    //port->is_rwc = 0;
    // FIS offset: 32K+256*portno
    // FIS entry size = 256 bytes per port
    port->fb = (uint64_t)(PORT_BASE + (32 << 10) + (portno << 8));
    memset((void *) (port->fb), 0, 256);
    //port->serr_rwc = 0xFFFFFFF;
    // Command table offset: 40K + 8K*portno
    // Command table size = 256*32 = 8K per port
    hba_cmd_header_t *cmdheader = (hba_cmd_header_t *)(port->clb);
    //kprintf("\nCommand header %p", cmdheader);
    for (int i = 0; i < 32; i++) {
        cmdheader[i].prdtl = 8;
        cmdheader[i].ctba = (uint64_t)(PORT_BASE + (40 << 10) + (i << 8));
        memset((void *) cmdheader[i].ctba, 0, 256);
    }



}

int find_cmdslot(hba_port_t *port) {
    // If not set in SACT and CI, the slot is free
    unsigned long slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & 1) == 0)
            return i;
        slots >>= 1;
    }
    kprintf("Cannot find free command list entry\n");
    return -1;
}

int write(hba_port_t *port, DWORD startl, DWORD starth, DWORD count, char *buf) {
    port->is_rwc = (DWORD) -1;               // Clear pending interrupt bits
    volatile int spin = 0; // Spin lock timeout counter
    int slot = find_cmdslot(port);
    if (slot == -1)
        return 0;

    hba_cmd_header_t *cmdheader = (hba_cmd_header_t *) port->clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(DWORD);   // Command FIS size
    cmdheader->w = 1;               // Read from device
    cmdheader->prdtl = (WORD) ((count - 1) >> 4) + 1;    // PRDT entries count

    hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t * )(cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t) +
                      (cmdheader->prdtl - 1) * sizeof(hba_prdt_entry_t));
    int i = 0;
    // 8K bytes (16 sectors) per PRDT
    for (i = 0; i < cmdheader->prdtl - 1; i++) {
        cmdtbl->prdt_entry[i].dba = (uint64_t) buf;
        cmdtbl->prdt_entry[i].dbc = 8 * 1024;     // 8K bytes
        cmdtbl->prdt_entry[i].i = 1;
        buf += 4 * 1024;  // 4K words
        count -= 16;    // 16 sectors
    }
    // Last entry
    cmdtbl->prdt_entry[i].dba = (uint64_t) buf;
    cmdtbl->prdt_entry[i].dbc = count << 9;   // 512 bytes per sector
    cmdtbl->prdt_entry[i].i = 1;

    // Setup command
    fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t * )(&cmdtbl->cfis);
    cmdfis->fis_type = 0x27;
    cmdfis->c = 1;  // Command
    cmdfis->command = 0x35;

    cmdfis->lba0 = (BYTE) startl;
    cmdfis->lba1 = (BYTE) (startl >> 8);
    cmdfis->lba2 = (BYTE) (startl >> 16);
    cmdfis->device = 1 << 6;  // LBA mode

    cmdfis->lba3 = (BYTE) (startl >> 24);
    cmdfis->lba4 = (BYTE) starth;
    cmdfis->lba5 = (BYTE) (starth >> 8);

    cmdfis->count = count;

    // The below loop waits until the port is no longer busy before issuing a new command
    //kprintf("\nValue of Task file %x ",port->tfd);
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
        spin++;
    }
    if (spin == 1000000) {
        kprintf("Port is hung\n");
        return 0;
    }

    port->ci = 1 << slot;     // Issue command

    // Wait for completion
    while (1) {
        // In some longer duration reads, it may be helpful to spin on the DPS bit
        // in the PxIS port field as well (1 << 5)
        if ((port->ci & (1 << slot)) == 0)
            break;
        if (port->is_rwc & HBA_PxIS_TFES)       // Task file error
        {
            kprintf("Write disk error\n");
            return 0;
        }
    }

    // Check again
    if (port->is_rwc & HBA_PxIS_TFES) {
        kprintf("Write disk error\n");
        return 0;
    }
    return 1;
}


int read(hba_port_t *port, DWORD startl, DWORD starth, DWORD count, char *buf) {
    port->is_rwc = (DWORD) -1;        // Clear pending interrupt bits
    volatile int spin = 0; // Spin lock timeout counter
    int slot = find_cmdslot(port);
    if (slot == -1)
        return 0;

    hba_cmd_header_t *cmdheader = (hba_cmd_header_t *) port->clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(DWORD);    // Command FIS size
    cmdheader->w = 0;        // Read from device
    cmdheader->prdtl = (WORD) ((count - 1) >> 4) + 1;    // PRDT entries count

    hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t * )(cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t) +
                      (cmdheader->prdtl - 1) * sizeof(hba_prdt_entry_t));
    int i = 0;
    // 8K bytes (16 sectors) per PRDT
    for (i = 0; i < cmdheader->prdtl - 1; i++) {
        cmdtbl->prdt_entry[i].dba = (uint64_t) buf;
        cmdtbl->prdt_entry[i].dbc = 8 * 1024;    // 8K bytes
        cmdtbl->prdt_entry[i].i = 1;
        buf += 4 * 1024;    // 4K words
        count -= 16;    // 16 sectors
    }
    // Last entry
    cmdtbl->prdt_entry[i].dba = (uint64_t) buf;
    cmdtbl->prdt_entry[i].dbc = count << 9;    // 512 bytes per sector
    cmdtbl->prdt_entry[i].i = 1;

    // Setup command
    fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t * )(&cmdtbl->cfis);

    cmdfis->fis_type = 0x27;
    cmdfis->c = 1;    // Command
    cmdfis->command = 0x25;

    cmdfis->lba0 = (BYTE) startl;
    cmdfis->lba1 = (BYTE) (startl >> 8);
    cmdfis->lba2 = (BYTE) (startl >> 16);
    cmdfis->device = 1 << 6;    // LBA mode

    cmdfis->lba3 = (BYTE) (startl >> 24);
    cmdfis->lba4 = (BYTE) starth;
    cmdfis->lba5 = (BYTE) (starth >> 8);

    cmdfis->count = count;

    // The below loop waits until the port is no longer busy before issuing a new command
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
        spin++;
    }
    if (spin == 1000000) {
        kprintf("Port is hung\n");
        return 0;
    }

    port->ci = 1 << slot;    // Issue command

    // Wait for completion
    while (1) {
        // In some longer duration reads, it may be helpful to spin on the DPS bit
        // in the PxIS port field as well (1 << 5)
        if ((port->ci & (1 << slot)) == 0)
            break;
        if (port->is_rwc & HBA_PxIS_TFES)    // Task file error
        {
            kprintf("Read disk error\n");
            return 0;
        }
    }

    // Check again
    if (port->is_rwc & HBA_PxIS_TFES) {
        kprintf("Read disk error\n");
        return 0;
    }

    return 1;
}

void delay(){
    volatile uint64_t cnt=0;
    while(cnt < 1000000){
        cnt++;
    }

}


void write_blocks(int num, hba_port_t *port) {
    int i = 0, j = 0, k = 0;
    char *writeptr;
    writeptr = (char *) (0x300000);
    uint8_t character;
    for (i = 0; i < num; i++) {
        character = i;
        j = 0;
        while (j < 512 * 8) {
            *writeptr = character;
            writeptr++;
            j++;
        }
        *writeptr = '\0';
        writeptr = (char *) (0x300000);
	delay();
	delay();
        write(port, k, 0, 8, writeptr);
        k += 8;
    }


}



void read_blocks(int num, hba_port_t *port) {
    char *readbuffer;
    int k;
    int j;
    int flag =0;
    readbuffer = (char *) (0x400000);
    kprintf("\n Reading from disk \n");
    for (k = 0; k < 100; k++) {
        read(port, k*8, 0, 8, readbuffer);
	uint8_t * readchar = (uint8_t *)readbuffer;
	j =0 ;
	flag =0;  
	while(j < 512 * 8){
	    if(*(readchar + j) != k){
		  flag =1;
		}
	    j ++;  	
	}
        if(flag == 0){
		kprintf("  %d",k);
        }
	else{
		kprintf(" ~%d",k);
	}
        kprintf(" %d ", (uint8_t) * readbuffer);
        delay();
    }


}

void reset_AHCI(hba_mem_t *abar) {

    /*if (abar->cap & 0x80000000) {
        kprintf("\n supports 64 bit addressing");
    }

    if (abar->cap & 0x08000000) {
        kprintf("\n supports staggered spin up");
    }*/

    //uint32_t ghcvalue = HBA_GHC_AE | HBA_GHC_IE |HBA_GHC_HR;

   abar->ghc |= HBA_GHC_HR;
    abar->ghc |= HBA_GHC_IE;
    abar->ghc |= HBA_GHC_AE;


    while (abar->ghc & 1);

}




void reset_port(hba_mem_t *abar,int portno){
    reset_AHCI(abar);
    stop_cmd(&abar->ports[portno]);
    port_rebase(&abar->ports[portno],portno);
    hba_port_t *port = &abar->ports[portno];
    //reset drive to active  by changing  DET bits and IPM to 0x301  
   
    port->sctl = 0x301;
    delay();
    port->sctl =0x300;
    delay();
    
    
    if(abar->cap & HBA_CAP_SSS){
        port->cmd |= (uint32_t)(HBA_PxCMD_ICC | HBA_PxCMD_POD | HBA_PxCMD_SUD);
	//spin up drive  power on the device and set device to active
        delay();
    }
    port->serr_rwc =0xFFFFFFFF;
    delay();
    port->is_rwc = 0xFFFFFFFF;
    delay();		
    start_cmd(port);
}





void probe_port(hba_mem_t *abar) {
    // Search disk in impelemented ports
    
    uint32_t pi = abar->pi;
    kprintf("\nValue of pi is %x", pi);
    int i = 0;
   
    while (i < 32) {
        if (pi & 1) {

            int dt = check_type(&abar->ports[i]);
            if (dt == 1 ) {
                
		  reset_port(abar,i);	
                   
                    write_blocks(100, &abar->ports[i]);
                    read_blocks(100, &abar->ports[i]);
                    kprintf("\nSATA drive found at port %d", i);
              
                    break;
                
            } else if (dt == 2) {
                kprintf("SATAPI drive found at port %d\n", i);
            } else if (dt == 3) {
                kprintf("SEMB drive found at port %d\n", i);
            } else if (dt == 4) {
                kprintf("PM drive found at port %d\n", i);
            } else {
                kprintf("No drive found at port %d\n", i);
            }
        }

        pi >>= 1;
        i++;
    }
}


void checkAllBuses() {
    uint8_t bus, device, func;
    uint16_t vendorvalue;
    uint16_t devicevalue;
    uint8_t class;
    uint8_t subclass;
    uint8_t offset;
    uint8_t progif;
    uint64_t abar2;
    uint64_t abar3;
    uint64_t abar;
    hba_mem_t *hbar;
    for (bus = 0; bus < 255; bus++) {
        for (device = 0; device < 32; device++) {
            for (func = 0; func < 8; func++) {
                offset = 0;
                vendorvalue = pciReadRegister(bus, device, func, offset);
                if (vendorvalue != 0xFFFF) {
                    devicevalue = pciReadRegister(bus, device, func, 2);
                    class = (uint8_t)((pciReadRegister(bus, device, func, 10) >> 8));
                    subclass = (uint8_t)((pciReadRegister(bus, device, func, 10) & 0x00FF));
                    progif = (uint8_t)(pciReadRegister(bus, device, func, 8) >> 8);
                    if (class == 0x01 && subclass == 0x06 && progif == 0x01){
                        kprintf("AHCI device found at bus %x with vendorid %x and deviceid %x \n", bus, vendorvalue,
                                devicevalue);
                        abar2 = (uint64_t)(pciReadRegister(bus, device, func, 36) & 0x000000000000FFFF);
                        abar3 = (uint64_t)(pciReadRegister(bus, device, func, 38) & 0x000000000000FFFF);
                        
                        outl(0xCFC, AHCI_BASE);
                        abar = (uint64_t)(AHCI_BASE);
                        hbar = (void *) abar;
                        //verify address with the one present in BAR5 register of the PCI interface
                        abar2 = (uint64_t)(pciReadRegister(bus, device, func, 36) & 0x000000000000FFFF);
                        abar3 = (uint64_t)(pciReadRegister(bus, device, func, 38) & 0x000000000000FFFF);
                        kprintf("HBAR %x", hbar);
                        kprintf("AHCI physical address %p", (abar3 << 16 | abar2));
                        probe_port(hbar);
                    }

                }
            }
        }

    }
}
