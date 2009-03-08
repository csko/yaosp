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
