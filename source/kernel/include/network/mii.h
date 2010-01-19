#ifndef _NETWORK_MII_H_
#define _NETWORK_MII_H_

/* Generic MII registers. */

#define MII_BMCR        0x00 /* Basic mode control register */
#define MII_BMSR        0x01 /* Basic mode status register  */
#define MII_PHYSID1     0x02 /* PHYS ID 1                   */
#define MII_PHYSID2     0x03 /* PHYS ID 2                   */
#define MII_ADVERTISE   0x04 /* Advertisement control reg   */
#define MII_LPA         0x05 /* Link partner ability reg    */
#define MII_EXPANSION   0x06 /* Expansion register          */
#define MII_CTRL1000    0x09 /* 1000BASE-T control          */
#define MII_STAT1000    0x0A /* 1000BASE-T status           */
#define MII_ESTATUS     0x0F /* Extended Status */
#define MII_DCOUNTER    0x12 /* Disconnect counter          */
#define MII_FCSCOUNTER  0x13 /* False carrier counter       */
#define MII_NWAYTEST    0x14 /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER 0x15 /* Receive error counter       */
#define MII_SREVISION   0x16 /* Silicon revision            */
#define MII_RESV1       0x17 /* Reserved...                 */
#define MII_LBRERROR    0x18 /* Lpback, rx, bypass error    */
#define MII_PHYADDR     0x19 /* PHY address                 */
#define MII_RESV2       0x1A /* Reserved...                 */
#define MII_TPISTATUS   0x1B /* TPI status for 10mbps       */
#define MII_NCONFIG     0x1C /* Network interface config    */

/* Advertisement control register. */

#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL     0x0020  /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF     0x0040  /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080  /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM 0x0100  /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP     0x0400  /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800  /* Try for asymetric pause     */
#define ADVERTISE_RESV          0x1000  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
                        ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)

/* 1000BASE-T Control register. */

#define ADVERTISE_1000FULL      0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF      0x0100  /* Advertise 1000BASE-T half duplex */

/* Basic mode control register. */

#define BMCR_RESV               0x003f  /* Unused...                   */
#define BMCR_SPEED1000          0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset the DP83840           */

typedef struct mii_if_info {
    int phy_id;
    int advertising;
    int phy_id_mask;
    int reg_num_mask;

    unsigned int full_duplex : 1;   /* is full duplex? */
    unsigned int force_media : 1;   /* is autoneg. disabled? */
    unsigned int supports_gmii : 1; /* are GMII registers supported? */
} mii_if_info_t;

#endif /* _NETWORK_MII_H_ */
