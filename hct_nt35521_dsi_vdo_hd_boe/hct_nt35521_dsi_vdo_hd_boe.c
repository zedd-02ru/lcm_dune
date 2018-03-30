
/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#ifdef BUILD_LK
#else
#include <linux/string.h>
#endif

#include "lcm_drv.h"
//yufeng
#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
        #include <mt-plat/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                          (720)
#define FRAME_HEIGHT                                         (1280)

#define REGFLAG_DELAY                                         0XEE
#define REGFLAG_END_OF_TABLE                                  0xFF   // END OF REGISTERS MARKER

#define LCM_ID_NT35521  0x5521
#define LCM_DSI_CMD_MODE                                    0

#define FALSE 0
#define TRUE 1
//HQ_fujin 131104
static unsigned int lcm_esd_test = FALSE;      ///only for ESD test
// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                      lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                  lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size) 

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{

		memset(params, 0, sizeof(LCM_PARAMS));

		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;
		//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

		params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;

		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		params->dsi.packet_size=256;

		// Video mode setting
		params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.word_count=720*3;


		params->dsi.vertical_sync_active				= 4;// 3    2
		params->dsi.vertical_backporch					= 38;// 20   1
		params->dsi.vertical_frontporch					= 40; // 1  12
		params->dsi.vertical_active_line				= FRAME_HEIGHT;

		params->dsi.horizontal_sync_active				= 10;// 50  2
		params->dsi.horizontal_backporch				= 72;
		params->dsi.horizontal_frontporch				= 72 ;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    //params->dsi.LPX=8;

		// Bit rate calculation
		//1 Every lane speed
        	//params->dsi.pll_select=1;
	    params->dsi.PLL_CLOCK = 195; //this value must be in MTK suggested table
    params->dsi.compatibility_for_nvk = 1;
}

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++) {
       
        unsigned cmd;
        cmd = table[i].cmd;
       
        switch (cmd) {

            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }

}

static struct LCM_setting_table lcm_initialization_setting[] = {
//REGFLAG_DELAY = 0xEE;
//REGFLAG_END_OF_TABLE = 0xFF;

//{0xf0, 5, {0x52,0x08,0x00}},
//{0xf0, 5, {0x55,0xaa,0x52,0x08,0x00}},

{0xff, 4, {0xaa,0x55,0xa5,0x80}},
{0x6f, 2, {0x11,0x00}},
{0xf7, 2, {0x20,0x00}},
{0x6f, 1, {0x01}},
{0xb1, 1, {0x21}},
{0xc8, 1, {0x80}},
{0xbd, 5, {0x01,0xa0,0x10,0x08,0x01}},
{0xb8, 4, {0x01,0x02,0x0c,0x02}},
{0xbb, 2, {0x11,0x11}},
{0xbc, 2, {0x00,0x00}},
{0xb6, 1, {0x02}},
{0xb9, 2, {0x13,0x13}},
{0xf0, 5, {0x55,0xaa,0x52,0x08,0x01}},
{0xb0, 2, {0x09,0x09}},
{0xb1, 2, {0x09,0x09}},
{0xbc, 2, {0x8c,0x00}},
{0xbd, 2, {0x8c,0x00}},
{0xca, 1, {0x00}},
{0xc0, 1, {0x0c}},
{0xb5, 2, {0x04,0x04}},
{0xbe, 1, {0x34}},
{0xb3, 2, {0x21,0x21}},
{0xb4, 2, {0x08,0x08}},
{0xb9, 2, {0x24,0x24}},
{0xba, 2, {0x36,0x36}},
{0xf0, 5, {0x55,0xaa,0x52,0x08,0x02}},

//{0xee, 1, {0x01}}, // for commit

{0xb0, 16, {0x00,0x4b,0x00,0x8b,0x00,0xbc,0x00,0xda,0x00,0xf2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xb1, 16, {0x01,0x88,0x01,0xc0,0x01,0xeb,0x02,0x2f,0x02,0x65,0x02,0x67,0x02,0x9a,0x02,0xd5}},
{0xb2, 16, {0x02,0xfc,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xa1,0x03,0xc4,0x03,0xd8,0x03,0xef}},
{0xb3, 4, {0x03,0xfe,0x03,0xff}},

{0xb4, 16, {0x00,0x4b,0x00,0x8b,0x00,0xbc,0x00,0xda,0x00,0xf2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xb5, 16, {0x01,0x88,0x01,0xc0,0x01,0xeb,0x02,0x2f,0x02,0x65,0x02,0x67,0x02,0x9a,0x02,0xd5}},
{0xb6, 16, {0x02,0xfc,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xa1,0x03,0xc4,0x03,0xd8,0x03,0xef}},
{0xb7, 4, {0x03,0xfe,0x03,0xff}},

{0xb8, 16, {0x00,0x4b,0x00,0x8b,0x00,0xbc,0x00,0xda,0x00,0xf2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xb9, 16, {0x01,0x88,0x01,0xc0,0x01,0xeb,0x02,0x2f,0x02,0x65,0x02,0x67,0x02,0x9a,0x02,0xd5}},
{0xba, 16, {0x02,0xfc,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xa1,0x03,0xc4,0x03,0xd8,0x03,0xef}},
{0xbb, 4, {0x03,0xfe,0x03,0xff}},

{0xbc, 16, {0x00,0x4b,0x00,0x8b,0x00,0xbc,0x00,0xda,0x00,0xf2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xbd, 16, {0x01,0x88,0x01,0xc0,0x01,0xeb,0x02,0x2f,0x02,0x65,0x02,0x67,0x02,0x9a,0x02,0xd5}},
{0xbe, 16, {0x02,0xfc,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xa1,0x03,0xc4,0x03,0xd8,0x03,0xef}},
{0xbf, 4, {0x03,0xfe,0x03,0xff}},

{0xc0, 16, {0x00,0x4b,0x00,0x8b,0x00,0xbc,0x00,0xda,0x00,0xf2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xc1, 16, {0x01,0x88,0x01,0xc0,0x01,0xeb,0x02,0x2f,0x02,0x65,0x02,0x67,0x02,0x9a,0x02,0xd5}},
{0xc2, 16, {0x02,0xfc,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xa1,0x03,0xc4,0x03,0xd8,0x03,0xef}},
{0xc3, 4, {0x03,0xfe,0x03,0xff}},

{0xc4, 16, {0x00,0x4b,0x00,0x8b,0x00,0xbc,0x00,0xda,0x00,0xf2,0x01,0x17,0x01,0x34,0x01,0x65}},
{0xc5, 16, {0x01,0x88,0x01,0xc0,0x01,0xeb,0x02,0x2f,0x02,0x65,0x02,0x67,0x02,0x9a,0x02,0xd5}},
{0xc6, 16, {0x02,0xfc,0x03,0x33,0x03,0x57,0x03,0x85,0x03,0xa1,0x03,0xc4,0x03,0xd8,0x03,0xef}},
{0xc7, 4, {0x03,0xfe,0x03,0xff}},

{0x6f, 1, {0x02}},
{0xf7, 1, {0x47}},
{0x6f, 1, {0x0a}},
{0xf7, 1, {0x02}},
{0x6f, 1, {0x17}},
{0xf4, 1, {0x60}},

{0xf0, 5, {0x55,0xaa,0x52,0x08,0x03}},
{0xb0, 2, {0x20,0x00}},
{0xb1, 2, {0x20,0x00}},
{0xb2, 5, {0x15,0x00,0x60,0x00,0x00}},
{0xb3, 5, {0x15,0x00,0x60,0x00,0x00}},
{0xb4, 5, {0x05,0x00,0x60,0x00,0x00}},
{0xb5, 5, {0x05,0x00,0x60,0x00,0x00}},
{0xba, 5, {0x44,0x10,0x60,0x01,0x90}},
{0xbb, 5, {0x44,0x10,0x60,0x01,0x90}},
{0xbc, 5, {0x44,0x10,0x60,0x01,0x90}},
{0xbd, 5, {0x44,0x10,0x60,0x01,0x90}},
{0xc0, 4, {0x00,0x34,0x00,0x00}},
{0xc1, 4, {0x00,0x00,0x34,0x00}},
{0xc2, 4, {0x00,0x00,0x34,0x00}},
{0xc3, 4, {0x00,0x00,0x34,0x00}},
{0xc4, 1, {0x60}},
{0xc5, 1, {0xc0}},
{0xc6, 1, {0x00}},
{0xc7, 1, {0x00}},
{0xf0, 5, {0x55,0xaa,0x52,0x08,0x05}},
{0xb0, 2, {0x17,0x06}},
{0xb1, 2, {0x17,0x06}},
{0xb2, 2, {0x17,0x06}},
{0xb3, 2, {0x17,0x06}},
{0xb4, 2, {0x17,0x06}},
{0xb5, 2, {0x17,0x06}},
{0xb6, 2, {0x14,0x03}},
{0xb7, 2, {0x00,0x00}},
{0xb8, 1, {0x0c}},
{0xb9, 2, {0x00,0x03}},
{0xba, 2, {0x00,0x01}},
{0xbb, 2, {0x0a,0x03}},
{0xbc, 2, {0x02,0x03}},
{0xbd, 5, {0x03,0x03,0x01,0x03,0x03}},
{0xc0, 1, {0x07}},
{0xc1, 1, {0x06}},
{0xc2, 1, {0xa6}},
{0xc3, 1, {0x05}},
{0xc4, 1, {0xa6}},
{0xc5, 1, {0xa6}},
{0xc6, 1, {0xa6}},
{0xc7, 1, {0xa6}},
{0xc8, 2, {0x05,0x20}},
{0xc9, 2, {0x04,0x20}},
{0xca, 2, {0x01,0x25}},
{0xcb, 2, {0x01,0x60}},
{0xcc, 3, {0x00,0x00,0x01}},
{0xcd, 3, {0x00,0x00,0x01}},
{0xce, 3, {0x00,0x00,0x02}},
{0xcf, 3, {0x00,0x00,0x02}},
{0xd0, 7, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0xd1, 5, {0x00,0x35,0x01,0x07,0x10}},
{0xd2, 5, {0x10,0x35,0x02,0x03,0x10}},
{0xd3, 5, {0x20,0x00,0x43,0x07,0x10}},
{0xd4, 5, {0x30,0x00,0x43,0x07,0x10}},
{0xd5, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},
{0x6f, 1, {0x06}},
{0xd5, 5, {0x00,0x00,0x00,0x00,0x00}},
{0xd6, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},
{0x6f, 1, {0x06}},
{0xd6, 5, {0x00,0x00,0x00,0x00,0x00}},
{0xd7, 6, {0x00,0x00,0x00,0x00,0x00,0x00}},
{0x6f, 1, {0x06}},
{0xd7, 5, {0x00,0x00,0x00,0x00,0x00}},
{0xd8, 5, {0x00,0x00,0x00,0x00,0x00}},
{0xe5, 1, {0x06}},
{0xe6, 1, {0x06}},
{0xe7, 1, {0x06}},
{0xe8, 1, {0x06}},
{0xe9, 1, {0x06}},
{0xea, 1, {0x06}},
{0xeb, 1, {0x00}},
{0xec, 1, {0x00}},
{0xed, 1, {0x30}},
{0xf0, 5, {0x55,0xaa,0x52,0x08,0x06}},
{0xb5, 2, {0x10,0x13}},
{0xb6, 2, {0x12,0x11}},
{0xb7, 2, {0x00,0x01}},
{0xb8, 2, {0x08,0x31}},
{0xb9, 2, {0x31,0x31}},
{0xba, 2, {0x31,0x31}},
{0xbb, 2, {0x31,0x08}},
{0xbc, 2, {0x03,0x02}},
{0xbd, 2, {0x17,0x18}},
{0xbe, 2, {0x19,0x16}},
{0xd8, 5, {0x00,0x00,0x00,0x00,0x00}},
{0xd9, 5, {0x00,0x00,0x00,0x00,0x00}},
{0xe5, 2, {0x00,0x00}},
{0xe7, 1, {0x00}},
{0x6f, 1, {0x01}},
{0xf9, 1, {0x46}},
{0x6f, 1, {0x11}},
{0xf3, 1, {0x01}},
{0xff, 4, {0xaa,0x55,0x25,0x01}},
{0x6f, 1, {0x16}},
{0xf7, 1, {0x10}},
{0x35, 1, {0x00}},


{0x11,1,{0x00}},
{REGFLAG_DELAY, 120, {}},
// Display ON
{0x29, 1, {0x00}},
//{REGFLAG_DELAY, 60, {}},
{REGFLAG_END_OF_TABLE, 0x00, {}},
	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.
	// Setting ending by predefined flag
};

static void lcm_init(void) //zedd
{
    unsigned int data_array[16];   
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(50);
    SET_RESET_PIN(1);
    MDELAY(120);
   // lcm_initialization();
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

/*
static void lcm_update(unsigned int x, unsigned int y,
        unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

    unsigned char x0_MSB = ((x0>>8)&0xFF);
    unsigned char x0_LSB = (x0&0xFF);
    unsigned char x1_MSB = ((x1>>8)&0xFF);
    unsigned char x1_LSB = (x1&0xFF);
    unsigned char y0_MSB = ((y0>>8)&0xFF);
    unsigned char y0_LSB = (y0&0xFF);
    unsigned char y1_MSB = ((y1>>8)&0xFF);
    unsigned char y1_LSB = (y1&0xFF);

    unsigned int data_array[16];

    data_array[0]= 0x00053902;
    data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
    data_array[2]= (x1_LSB);
    dsi_set_cmdq(data_array, 3, 1);
    //MDELAY(1);
   
    data_array[0]= 0x00053902;
    data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[2]= (y1_LSB);
    dsi_set_cmdq(data_array, 3, 1);
    //MDELAY(1);
   
    data_array[0]= 0x00290508;
    dsi_set_cmdq(data_array, 1, 1);
    //MDELAY(1);
   
    data_array[0]= 0x002c3909;
    dsi_set_cmdq(data_array, 1, 0);
    //MDELAY(1);

}
*/

static void lcm_suspend(void)
{
    unsigned int data_array[16];
    data_array[0]=0x00280500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(10);
    data_array[0]=0x00100500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(100);
}


static void lcm_resume(void)
{
/*    unsigned int data_array[16];
    data_array[0]=0x00110500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(100);
    data_array[0]=0x00290500;
    dsi_set_cmdq(data_array,1,1);
    MDELAY(10);
 */
 return lcm_init_();
}


static unsigned int lcm_compare_id(void)
{
/* 
    unsigned int id=0;
    unsigned char buffer[3];
    unsigned int array[16]; 
    unsigned int data_array[16];

    SET_RESET_PIN(1);
        MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(50);
   
    SET_RESET_PIN(1);
    MDELAY(120);

    data_array[0] = 0x00063902;
    data_array[1] = 0x52AA55F0; 
    data_array[2] = 0x00000108;               
    dsi_set_cmdq(data_array, 3, 1);

    array[0] = 0x00033700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
   
    read_reg_v2(0xC5, buffer, 3);
    id = buffer[1]; //we only need ID
    #ifdef BUILD_LK
        printf("%s, LK nt35590 debug: nt35590 id = 0x%08x buffer[0]=0x%08x,buffer[1]=0x%08x,buffer[2]=0x%08x\n", __func__, id,buffer[0],buffer[1],buffer[2]);
    #else
        printk("%s, LK nt35590 debug: nt35590 id = 0x%08x buffer[0]=0x%08x,buffer[1]=0x%08x,buffer[2]=0x%08x\n", __func__, id,buffer[0],buffer[1],buffer[2]);
    #endif
*/
   // if(id == LCM_ID_NT35521)

//    if(buffer[0]==0x55 && buffer[1]==0x21)
        return TRUE;
//    else
//        return 0;


}


// static int err_count = 0; zedd

/* zedd
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
    unsigned char buffer[8] = {0};
    unsigned int array[4];
    int i =0;

    array[0] = 0x00013700;   
    dsi_set_cmdq(array, 1,1);
    read_reg_v2(0x0A, buffer,8);

    printk( "nt35521_JDI lcm_esd_check: buffer[0] = %d,buffer[1] = %d,buffer[2] = %d,buffer[3] = %d,buffer[4] = %d,buffer[5] = %d,buffer[6] = %d,buffer[7] = %d\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);

    if((buffer[0] != 0x9C)) //LCD work status error,need re-initalize
    {
        printk( "nt35521_JDI lcm_esd_check buffer[0] = %d\n",buffer[0]);
        return TRUE;
    }
    else
    {
        if(buffer[3] != 0x02) //error data type is 0x02
        {
             //return FALSE;
        err_count = 0;
        }
        else
        {
             //if(((buffer[4] != 0) && (buffer[4] != 0x40)) ||  (buffer[5] != 0x80))
        if( (buffer[4] == 0x40) || (buffer[5] == 0x80))
             {
                  err_count = 0;
             }
             else
             {
                  err_count++;
             }            
             if(err_count >=2 )
             {
                 err_count = 0;
                 printk( "nt35521_JDI lcm_esd_check buffer[4] = %d , buffer[5] = %d\n",buffer[4],buffer[5]);
                 return TRUE;
             }
        }
        return FALSE;
    }
#endif
   
}
*/

static unsigned int lcm_esd_recover(void)
{
    lcm_init();
    return TRUE;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hct_nt35521_dsi_vdo_hd_boe= 
{
    .name			= "hct_nt35521_dsi_vdo_hd_boe",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id    = lcm_compare_id,
#if 0//defined(LCM_DSI_CMD_MODE)
//    .set_backlight  = lcm_setbacklight,
//    .set_pwm        = lcm_setpwm,
//    .get_pwm        = lcm_getpwm,
//    .update         = lcm_update
#endif
};

