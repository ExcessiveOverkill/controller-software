`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Excessive Overkill
// Engineer: Stacey Hdlforbeginners
// 
// Create Date: 08/12/2023 10:50:04 AM
// Design Name: Controller Software
// Module Name: controller_firmware
// Project Name: Excessive Motion
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module controller_firmware_top
  (

   inout [14:0]	DDR_addr,
   inout [2:0]	DDR_ba,
   inout	DDR_cas_n,
   inout	DDR_ck_n,
   inout	DDR_ck_p,
   inout	DDR_cke,
   inout	DDR_cs_n,
   inout [3:0]	DDR_dm,
   inout [31:0]	DDR_dq,
   inout [3:0]	DDR_dqs_n,
   inout [3:0]	DDR_dqs_p,
   inout	DDR_odt,
   inout	DDR_ras_n,
   inout	DDR_reset_n,
   inout	DDR_we_n,
   inout	FIXED_IO_ddr_vrn,
   inout	FIXED_IO_ddr_vrp,
   inout [53:0]	FIXED_IO_mio,
   inout	FIXED_IO_ps_clk,
   inout	FIXED_IO_ps_porb,
   inout	FIXED_IO_ps_srstb,
   
   inout [21:0] SLOT_A,
   output [2:0] RGB_LED,
   output BUZZER
   
//   wire [21:0] SLOT_A_OUT,
//   wire [21:0] SLOT_A_IN,
//   wire [21:0] SLOT_A_OUTEN
   
   
  );
  
    wire [21:0] SLOT_A_OUT;
    wire [21:0] SLOT_A_IN;
    wire [21:0] SLOT_A_OUTEN;
   
   controller_firmware_wrapper controller_firmware_wrapper_i
     (
      
      .DDR_addr(DDR_addr),
      .DDR_ba(DDR_ba),
      .DDR_cas_n(DDR_cas_n),
      .DDR_ck_n(DDR_ck_n),
      .DDR_ck_p(DDR_ck_p),
      .DDR_cke(DDR_cke),
      .DDR_cs_n(DDR_cs_n),
      .DDR_dm(DDR_dm),
      .DDR_dq(DDR_dq),
      .DDR_dqs_n(DDR_dqs_n),
      .DDR_dqs_p(DDR_dqs_p),
      .DDR_odt(DDR_odt),
      .DDR_ras_n(DDR_ras_n),
      .DDR_reset_n(DDR_reset_n),
      .DDR_we_n(DDR_we_n),
      .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn),
      .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
      .FIXED_IO_mio(FIXED_IO_mio),
      .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
      .FIXED_IO_ps_porb(FIXED_IO_ps_porb),
      .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
      .RGB_LED(RGB_LED),
      .SLOT_A_OUT(SLOT_A_OUT),
      .SLOT_A_IN(SLOT_A_IN),
      .SLOT_A_OUTEN(SLOT_A_OUTEN),
      .BUZZER(BUZZER)

      );
      
    IOBUF SLOT_A_0_IOBUF
       (.O(SLOT_A_IN[0]),
        .IO(SLOT_A[0]),
        .I(SLOT_A_OUT[0]),
        .T(SLOT_A_OUTEN[0]));
        
    IOBUF SLOT_A_1_IOBUF
       (.O(SLOT_A_IN[1]),
        .IO(SLOT_A[1]),
        .I(SLOT_A_OUT[1]),
        .T(SLOT_A_OUTEN[1]));
        
    IOBUF SLOT_A_2_IOBUF
       (.O(SLOT_A_IN[2]),
        .IO(SLOT_A[2]),
        .I(SLOT_A_OUT[2]),
        .T(SLOT_A_OUTEN[2]));
        
    IOBUF SLOT_A_3_IOBUF
       (.O(SLOT_A_IN[3]),
        .IO(SLOT_A[3]),
        .I(SLOT_A_OUT[3]),
        .T(SLOT_A_OUTEN[3]));
        
    IOBUF SLOT_A_4_IOBUF
       (.O(SLOT_A_IN[4]),
        .IO(SLOT_A[4]),
        .I(SLOT_A_OUT[4]),
        .T(SLOT_A_OUTEN[4]));
        
    IOBUF SLOT_A_5_IOBUF
       (.O(SLOT_A_IN[5]),
        .IO(SLOT_A[5]),
        .I(SLOT_A_OUT[5]),
        .T(SLOT_A_OUTEN[5]));
        
    IOBUF SLOT_A_6_IOBUF
       (.O(SLOT_A_IN[6]),
        .IO(SLOT_A[6]),
        .I(SLOT_A_OUT[6]),
        .T(SLOT_A_OUTEN[6]));
        
    IOBUF SLOT_A_7_IOBUF
       (.O(SLOT_A_IN[7]),
        .IO(SLOT_A[7]),
        .I(SLOT_A_OUT[7]),
        .T(SLOT_A_OUTEN[7]));
        
    IOBUF SLOT_A_8_IOBUF
       (.O(SLOT_A_IN[8]),
        .IO(SLOT_A[8]),
        .I(SLOT_A_OUT[8]),
        .T(SLOT_A_OUTEN[8]));
        
    IOBUF SLOT_A_9_IOBUF
       (.O(SLOT_A_IN[9]),
        .IO(SLOT_A[9]),
        .I(SLOT_A_OUT[9]),
        .T(SLOT_A_OUTEN[9]));
        
    IOBUF SLOT_A_10_IOBUF
       (.O(SLOT_A_IN[10]),
        .IO(SLOT_A[10]),
        .I(SLOT_A_OUT[10]),
        .T(SLOT_A_OUTEN[10]));
        
    IOBUF SLOT_A_11_IOBUF
       (.O(SLOT_A_IN[11]),
        .IO(SLOT_A[11]),
        .I(SLOT_A_OUT[11]),
        .T(SLOT_A_OUTEN[11]));
        
    IOBUF SLOT_A_12_IOBUF
       (.O(SLOT_A_IN[12]),
        .IO(SLOT_A[12]),
        .I(SLOT_A_OUT[12]),
        .T(SLOT_A_OUTEN[12]));
        
    IOBUF SLOT_A_13_IOBUF
       (.O(SLOT_A_IN[13]),
        .IO(SLOT_A[13]),
        .I(SLOT_A_OUT[13]),
        .T(SLOT_A_OUTEN[13]));
        
    IOBUF SLOT_A_14_IOBUF
       (.O(SLOT_A_IN[14]),
        .IO(SLOT_A[14]),
        .I(SLOT_A_OUT[14]),
        .T(SLOT_A_OUTEN[14]));
        
    IOBUF SLOT_A_15_IOBUF
       (.O(SLOT_A_IN[15]),
        .IO(SLOT_A[15]),
        .I(SLOT_A_OUT[15]),
        .T(SLOT_A_OUTEN[15]));
        
    IOBUF SLOT_A_16_IOBUF
       (.O(SLOT_A_IN[16]),
        .IO(SLOT_A[16]),
        .I(SLOT_A_OUT[16]),
        .T(SLOT_A_OUTEN[16]));
        
    IOBUF SLOT_A_17_IOBUF
       (.O(SLOT_A_IN[17]),
        .IO(SLOT_A[17]),
        .I(SLOT_A_OUT[17]),
        .T(SLOT_A_OUTEN[17]));
        
    IOBUF SLOT_A_18_IOBUF
       (.O(SLOT_A_IN[18]),
        .IO(SLOT_A[18]),
        .I(SLOT_A_OUT[18]),
        .T(SLOT_A_OUTEN[18]));
        
    IOBUF SLOT_A_19_IOBUF
       (.O(SLOT_A_IN[19]),
        .IO(SLOT_A[19]),
        .I(SLOT_A_OUT[19]),
        .T(SLOT_A_OUTEN[19]));
 
    IOBUF SLOT_A_20_IOBUF
       (.O(SLOT_A_IN[20]),
        .IO(SLOT_A[20]),
        .I(SLOT_A_OUT[20]),
        .T(SLOT_A_OUTEN[20]));
        
    IOBUF SLOT_A_21_IOBUF
       (.O(SLOT_A_IN[21]),
        .IO(SLOT_A[21]),
        .I(SLOT_A_OUT[21]),
        .T(SLOT_A_OUTEN[21]));
 
//    assign SLOT_A[0] = SLOT_A_OUTEN[0]? SLOT_A_OUT[0] : 1'bZ;;
//    assign SLOT_A_IN[0] = SLOT_A[0];
    
//    assign SLOT_A[1] = SLOT_A_OUTEN[1]? SLOT_A_OUT[1] : 1'bZ;;
//    assign SLOT_A_IN[1] = SLOT_A[1];
    
//    assign SLOT_A[2] = SLOT_A_OUTEN[2]? SLOT_A_OUT[2] : 1'bZ;;
//    assign SLOT_A_IN[2] = SLOT_A[2];
    
//    assign SLOT_A[3] = SLOT_A_OUTEN[3]? SLOT_A_OUT[3] : 1'bZ;;
//    assign SLOT_A_IN[3] = SLOT_A[3];
    
//    assign SLOT_A[4] = SLOT_A_OUTEN[4]? SLOT_A_OUT[4] : 1'bZ;;
//    assign SLOT_A_IN[4] = SLOT_A[4];
    
//    assign SLOT_A[5] = SLOT_A_OUTEN[5]? SLOT_A_OUT[5] : 1'bZ;;
//    assign SLOT_A_IN[5] = SLOT_A[5];
    
//    assign SLOT_A[6] = SLOT_A_OUTEN[6]? SLOT_A_OUT[6] : 1'bZ;;
//    assign SLOT_A_IN[6] = SLOT_A[6];
    
//    assign SLOT_A[7] = SLOT_A_OUTEN[7]? SLOT_A_OUT[7] : 1'bZ;;
//    assign SLOT_A_IN[7] = SLOT_A[7];
    
//    assign SLOT_A[8] = SLOT_A_OUTEN[8]? SLOT_A_OUT[8] : 1'bZ;;
//    assign SLOT_A_IN[8] = SLOT_A[8];
    
//    assign SLOT_A[9] = SLOT_A_OUTEN[9]? SLOT_A_OUT[9] : 1'bZ;;
//    assign SLOT_A_IN[9] = SLOT_A[9];
    
//    assign SLOT_A[10] = SLOT_A_OUTEN[10]? SLOT_A_OUT[10] : 1'bZ;;
//    assign SLOT_A_IN[10] = SLOT_A[10];
 
//    assign SLOT_A[11] = SLOT_A_OUTEN[11]? SLOT_A_OUT[11] : 1'bZ;;
//    assign SLOT_A_IN[11] = SLOT_A[11];
    
//    assign SLOT_A[12] = SLOT_A_OUTEN[12]? SLOT_A_OUT[12] : 1'bZ;;
//    assign SLOT_A_IN[12] = SLOT_A[12];
    
//    assign SLOT_A[13] = SLOT_A_OUTEN[13]? SLOT_A_OUT[13] : 1'bZ;;
//    assign SLOT_A_IN[13] = SLOT_A[13];
    
//    assign SLOT_A[14] = SLOT_A_OUTEN[14]? SLOT_A_OUT[14] : 1'bZ;;
//    assign SLOT_A_IN[14] = SLOT_A[14];
    
//    assign SLOT_A[15] = SLOT_A_OUTEN[15]? SLOT_A_OUT[15] : 1'bZ;;
//    assign SLOT_A_IN[15] = SLOT_A[15];
    
//    assign SLOT_A[16] = SLOT_A_OUTEN[16]? SLOT_A_OUT[16] : 1'bZ;;
//    assign SLOT_A_IN[16] = SLOT_A[16];
    
//    assign SLOT_A[17] = SLOT_A_OUTEN[17]? SLOT_A_OUT[17] : 1'bZ;;
//    assign SLOT_A_IN[17] = SLOT_A[17];
    
//    assign SLOT_A[18] = SLOT_A_OUTEN[18]? SLOT_A_OUT[18] : 1'bZ;;
//    assign SLOT_A_IN[18] = SLOT_A[18];
    
//    assign SLOT_A[19] = SLOT_A_OUTEN[19]? SLOT_A_OUT[19] : 1'bZ;;
//    assign SLOT_A_IN[19] = SLOT_A[19];
    
//    assign SLOT_A[20] = SLOT_A_OUTEN[20]? SLOT_A_OUT[20] : 1'bZ;;
//    assign SLOT_A_IN[20] = SLOT_A[20];
    
//    assign SLOT_A[21] = SLOT_A_OUTEN[21]? SLOT_A_OUT[21] : 1'bZ;;
//    assign SLOT_A_IN[21] = SLOT_A[21];


endmodule


