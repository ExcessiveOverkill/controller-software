`timescale 1ns / 1ps

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
   inout [21:0] SLOT_B,
   inout [21:0] SLOT_C,
   inout [21:0] SLOT_D,
//    output [2:0] RGB_LED,
   output BUZZER
   
   
   
  );
  
   wire [21:0] SLOT_A_OUT;
   wire [21:0] SLOT_A_IN;
   wire [21:0] SLOT_A_OUTEN;

   wire [21:0] SLOT_B_OUT;
   wire [21:0] SLOT_B_IN;
   wire [21:0] SLOT_B_OUTEN;

   wire [21:0] SLOT_C_OUT;
   wire [21:0] SLOT_C_IN;
   wire [21:0] SLOT_C_OUTEN;

   wire [21:0] SLOT_D_OUT;
   wire [21:0] SLOT_D_IN;
   wire [21:0] SLOT_D_OUTEN;
   
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
      .SLOT_A_OUT(SLOT_A_OUT),
      .SLOT_A_IN(SLOT_A_IN),
      .SLOT_A_OUTEN(SLOT_A_OUTEN),
      .SLOT_B_OUT(SLOT_B_OUT),
      .SLOT_B_IN(SLOT_B_IN),
      .SLOT_B_OUTEN(SLOT_B_OUTEN),
      .SLOT_C_OUT(SLOT_C_OUT),
      .SLOT_C_IN(SLOT_C_IN),
      .SLOT_C_OUTEN(SLOT_C_OUTEN),
      .SLOT_D_OUT(SLOT_D_OUT),
      .SLOT_D_IN(SLOT_D_IN),
      .SLOT_D_OUTEN(SLOT_D_OUTEN),
    //   .RGB_LED(RGB_LED),
      .BUZZER(BUZZER)

      );
      
    IOBUF SLOT_A_0_IOBUF
       (.O(SLOT_A_IN[0]),
        .IO(SLOT_A[0]),
        .I(SLOT_A_OUT[0]),
        .T(~SLOT_A_OUTEN[0]));
        
    IOBUF SLOT_A_1_IOBUF
       (.O(SLOT_A_IN[1]),
        .IO(SLOT_A[1]),
        .I(SLOT_A_OUT[1]),
        .T(~SLOT_A_OUTEN[1]));
        
    IOBUF SLOT_A_2_IOBUF
       (.O(SLOT_A_IN[2]),
        .IO(SLOT_A[2]),
        .I(SLOT_A_OUT[2]),
        .T(~SLOT_A_OUTEN[2]));
        
    IOBUF SLOT_A_3_IOBUF
       (.O(SLOT_A_IN[3]),
        .IO(SLOT_A[3]),
        .I(SLOT_A_OUT[3]),
        .T(~SLOT_A_OUTEN[3]));
        
    IOBUF SLOT_A_4_IOBUF
       (.O(SLOT_A_IN[4]),
        .IO(SLOT_A[4]),
        .I(SLOT_A_OUT[4]),
        .T(~SLOT_A_OUTEN[4]));
        
    IOBUF SLOT_A_5_IOBUF
       (.O(SLOT_A_IN[5]),
        .IO(SLOT_A[5]),
        .I(SLOT_A_OUT[5]),
        .T(~SLOT_A_OUTEN[5]));
        
    IOBUF SLOT_A_6_IOBUF
       (.O(SLOT_A_IN[6]),
        .IO(SLOT_A[6]),
        .I(SLOT_A_OUT[6]),
        .T(~SLOT_A_OUTEN[6]));
        
    IOBUF SLOT_A_7_IOBUF
       (.O(SLOT_A_IN[7]),
        .IO(SLOT_A[7]),
        .I(SLOT_A_OUT[7]),
        .T(~SLOT_A_OUTEN[7]));
        
    IOBUF SLOT_A_8_IOBUF
       (.O(SLOT_A_IN[8]),
        .IO(SLOT_A[8]),
        .I(SLOT_A_OUT[8]),
        .T(~SLOT_A_OUTEN[8]));
        
    IOBUF SLOT_A_9_IOBUF
       (.O(SLOT_A_IN[9]),
        .IO(SLOT_A[9]),
        .I(SLOT_A_OUT[9]),
        .T(~SLOT_A_OUTEN[9]));
        
    IOBUF SLOT_A_10_IOBUF
       (.O(SLOT_A_IN[10]),
        .IO(SLOT_A[10]),
        .I(SLOT_A_OUT[10]),
        .T(~SLOT_A_OUTEN[10]));
        
    IOBUF SLOT_A_11_IOBUF
       (.O(SLOT_A_IN[11]),
        .IO(SLOT_A[11]),
        .I(SLOT_A_OUT[11]),
        .T(~SLOT_A_OUTEN[11]));
        
    IOBUF SLOT_A_12_IOBUF
       (.O(SLOT_A_IN[12]),
        .IO(SLOT_A[12]),
        .I(SLOT_A_OUT[12]),
        .T(~SLOT_A_OUTEN[12]));
        
    IOBUF SLOT_A_13_IOBUF
       (.O(SLOT_A_IN[13]),
        .IO(SLOT_A[13]),
        .I(SLOT_A_OUT[13]),
        .T(~SLOT_A_OUTEN[13]));
        
    IOBUF SLOT_A_14_IOBUF
       (.O(SLOT_A_IN[14]),
        .IO(SLOT_A[14]),
        .I(SLOT_A_OUT[14]),
        .T(~SLOT_A_OUTEN[14]));
        
    IOBUF SLOT_A_15_IOBUF
       (.O(SLOT_A_IN[15]),
        .IO(SLOT_A[15]),
        .I(SLOT_A_OUT[15]),
        .T(~SLOT_A_OUTEN[15]));
        
    IOBUF SLOT_A_16_IOBUF
       (.O(SLOT_A_IN[16]),
        .IO(SLOT_A[16]),
        .I(SLOT_A_OUT[16]),
        .T(~SLOT_A_OUTEN[16]));
        
    IOBUF SLOT_A_17_IOBUF
       (.O(SLOT_A_IN[17]),
        .IO(SLOT_A[17]),
        .I(SLOT_A_OUT[17]),
        .T(~SLOT_A_OUTEN[17]));
        
    IOBUF SLOT_A_18_IOBUF
       (.O(SLOT_A_IN[18]),
        .IO(SLOT_A[18]),
        .I(SLOT_A_OUT[18]),
        .T(~SLOT_A_OUTEN[18]));
        
    IOBUF SLOT_A_19_IOBUF
       (.O(SLOT_A_IN[19]),
        .IO(SLOT_A[19]),
        .I(SLOT_A_OUT[19]),
        .T(~SLOT_A_OUTEN[19]));
 
    IOBUF SLOT_A_20_IOBUF
       (.O(SLOT_A_IN[20]),
        .IO(SLOT_A[20]),
        .I(SLOT_A_OUT[20]),
        .T(~SLOT_A_OUTEN[20]));
        
    IOBUF SLOT_A_21_IOBUF
       (.O(SLOT_A_IN[21]),
        .IO(SLOT_A[21]),
        .I(SLOT_A_OUT[21]),
        .T(~SLOT_A_OUTEN[21]));


   IOBUF SLOT_B_0_IOBUF
       (.O(SLOT_B_IN[0]),
        .IO(SLOT_B[0]),
        .I(SLOT_B_OUT[0]),
        .T(~SLOT_B_OUTEN[0]));
        
    IOBUF SLOT_B_1_IOBUF
       (.O(SLOT_B_IN[1]),
        .IO(SLOT_B[1]),
        .I(SLOT_B_OUT[1]),
        .T(~SLOT_B_OUTEN[1]));
        
    IOBUF SLOT_B_2_IOBUF
       (.O(SLOT_B_IN[2]),
        .IO(SLOT_B[2]),
        .I(SLOT_B_OUT[2]),
        .T(~SLOT_B_OUTEN[2]));
        
    IOBUF SLOT_B_3_IOBUF
       (.O(SLOT_B_IN[3]),
        .IO(SLOT_B[3]),
        .I(SLOT_B_OUT[3]),
        .T(~SLOT_B_OUTEN[3]));
        
    IOBUF SLOT_B_4_IOBUF
       (.O(SLOT_B_IN[4]),
        .IO(SLOT_B[4]),
        .I(SLOT_B_OUT[4]),
        .T(~SLOT_B_OUTEN[4]));
        
    IOBUF SLOT_B_5_IOBUF
       (.O(SLOT_B_IN[5]),
        .IO(SLOT_B[5]),
        .I(SLOT_B_OUT[5]),
        .T(~SLOT_B_OUTEN[5]));
        
    IOBUF SLOT_B_6_IOBUF
       (.O(SLOT_B_IN[6]),
        .IO(SLOT_B[6]),
        .I(SLOT_B_OUT[6]),
        .T(~SLOT_B_OUTEN[6]));
        
    IOBUF SLOT_B_7_IOBUF
       (.O(SLOT_B_IN[7]),
        .IO(SLOT_B[7]),
        .I(SLOT_B_OUT[7]),
        .T(~SLOT_B_OUTEN[7]));
        
    IOBUF SLOT_B_8_IOBUF
       (.O(SLOT_B_IN[8]),
        .IO(SLOT_B[8]),
        .I(SLOT_B_OUT[8]),
        .T(~SLOT_B_OUTEN[8]));
        
    IOBUF SLOT_B_9_IOBUF
       (.O(SLOT_B_IN[9]),
        .IO(SLOT_B[9]),
        .I(SLOT_B_OUT[9]),
        .T(~SLOT_B_OUTEN[9]));
        
    IOBUF SLOT_B_10_IOBUF
       (.O(SLOT_B_IN[10]),
        .IO(SLOT_B[10]),
        .I(SLOT_B_OUT[10]),
        .T(~SLOT_B_OUTEN[10]));
        
    IOBUF SLOT_B_11_IOBUF
       (.O(SLOT_B_IN[11]),
        .IO(SLOT_B[11]),
        .I(SLOT_B_OUT[11]),
        .T(~SLOT_B_OUTEN[11]));
        
    IOBUF SLOT_B_12_IOBUF
       (.O(SLOT_B_IN[12]),
        .IO(SLOT_B[12]),
        .I(SLOT_B_OUT[12]),
        .T(~SLOT_B_OUTEN[12]));
        
    IOBUF SLOT_B_13_IOBUF
       (.O(SLOT_B_IN[13]),
        .IO(SLOT_B[13]),
        .I(SLOT_B_OUT[13]),
        .T(~SLOT_B_OUTEN[13]));
        
    IOBUF SLOT_B_14_IOBUF
       (.O(SLOT_B_IN[14]),
        .IO(SLOT_B[14]),
        .I(SLOT_B_OUT[14]),
        .T(~SLOT_B_OUTEN[14]));
        
    IOBUF SLOT_B_15_IOBUF
       (.O(SLOT_B_IN[15]),
        .IO(SLOT_B[15]),
        .I(SLOT_B_OUT[15]),
        .T(~SLOT_B_OUTEN[15]));
        
    IOBUF SLOT_B_16_IOBUF
       (.O(SLOT_B_IN[16]),
        .IO(SLOT_B[16]),
        .I(SLOT_B_OUT[16]),
        .T(~SLOT_B_OUTEN[16]));
        
    IOBUF SLOT_B_17_IOBUF
       (.O(SLOT_B_IN[17]),
        .IO(SLOT_B[17]),
        .I(SLOT_B_OUT[17]),
        .T(~SLOT_B_OUTEN[17]));
        
    IOBUF SLOT_B_18_IOBUF
       (.O(SLOT_B_IN[18]),
        .IO(SLOT_B[18]),
        .I(SLOT_B_OUT[18]),
        .T(~SLOT_B_OUTEN[18]));
        
    IOBUF SLOT_B_19_IOBUF
       (.O(SLOT_B_IN[19]),
        .IO(SLOT_B[19]),
        .I(SLOT_B_OUT[19]),
        .T(~SLOT_B_OUTEN[19]));
 
    IOBUF SLOT_B_20_IOBUF
       (.O(SLOT_B_IN[20]),
        .IO(SLOT_B[20]),
        .I(SLOT_B_OUT[20]),
        .T(~SLOT_B_OUTEN[20]));
        
    IOBUF SLOT_B_21_IOBUF
       (.O(SLOT_B_IN[21]),
        .IO(SLOT_B[21]),
        .I(SLOT_B_OUT[21]),
        .T(~SLOT_B_OUTEN[21]));
   

   IOBUF SLOT_C_0_IOBUF
       (.O(SLOT_C_IN[0]),
        .IO(SLOT_C[0]),
        .I(SLOT_C_OUT[0]),
        .T(~SLOT_C_OUTEN[0]));
        
    IOBUF SLOT_C_1_IOBUF
       (.O(SLOT_C_IN[1]),
        .IO(SLOT_C[1]),
        .I(SLOT_C_OUT[1]),
        .T(~SLOT_C_OUTEN[1]));
        
    IOBUF SLOT_C_2_IOBUF
       (.O(SLOT_C_IN[2]),
        .IO(SLOT_C[2]),
        .I(SLOT_C_OUT[2]),
        .T(~SLOT_C_OUTEN[2]));
        
    IOBUF SLOT_C_3_IOBUF
       (.O(SLOT_C_IN[3]),
        .IO(SLOT_C[3]),
        .I(SLOT_C_OUT[3]),
        .T(~SLOT_C_OUTEN[3]));
        
    IOBUF SLOT_C_4_IOBUF
       (.O(SLOT_C_IN[4]),
        .IO(SLOT_C[4]),
        .I(SLOT_C_OUT[4]),
        .T(~SLOT_C_OUTEN[4]));
        
    IOBUF SLOT_C_5_IOBUF
       (.O(SLOT_C_IN[5]),
        .IO(SLOT_C[5]),
        .I(SLOT_C_OUT[5]),
        .T(~SLOT_C_OUTEN[5]));
        
    IOBUF SLOT_C_6_IOBUF
       (.O(SLOT_C_IN[6]),
        .IO(SLOT_C[6]),
        .I(SLOT_C_OUT[6]),
        .T(~SLOT_C_OUTEN[6]));
        
    IOBUF SLOT_C_7_IOBUF
       (.O(SLOT_C_IN[7]),
        .IO(SLOT_C[7]),
        .I(SLOT_C_OUT[7]),
        .T(~SLOT_C_OUTEN[7]));
        
    IOBUF SLOT_C_8_IOBUF
       (.O(SLOT_C_IN[8]),
        .IO(SLOT_C[8]),
        .I(SLOT_C_OUT[8]),
        .T(~SLOT_C_OUTEN[8]));
        
    IOBUF SLOT_C_9_IOBUF
       (.O(SLOT_C_IN[9]),
        .IO(SLOT_C[9]),
        .I(SLOT_C_OUT[9]),
        .T(~SLOT_C_OUTEN[9]));
        
    IOBUF SLOT_C_10_IOBUF
       (.O(SLOT_C_IN[10]),
        .IO(SLOT_C[10]),
        .I(SLOT_C_OUT[10]),
        .T(~SLOT_C_OUTEN[10]));
        
    IOBUF SLOT_C_11_IOBUF
       (.O(SLOT_C_IN[11]),
        .IO(SLOT_C[11]),
        .I(SLOT_C_OUT[11]),
        .T(~SLOT_C_OUTEN[11]));
        
    IOBUF SLOT_C_12_IOBUF
       (.O(SLOT_C_IN[12]),
        .IO(SLOT_C[12]),
        .I(SLOT_C_OUT[12]),
        .T(~SLOT_C_OUTEN[12]));
        
    IOBUF SLOT_C_13_IOBUF
       (.O(SLOT_C_IN[13]),
        .IO(SLOT_C[13]),
        .I(SLOT_C_OUT[13]),
        .T(~SLOT_C_OUTEN[13]));
        
    IOBUF SLOT_C_14_IOBUF
       (.O(SLOT_C_IN[14]),
        .IO(SLOT_C[14]),
        .I(SLOT_C_OUT[14]),
        .T(~SLOT_C_OUTEN[14]));
        
    IOBUF SLOT_C_15_IOBUF
       (.O(SLOT_C_IN[15]),
        .IO(SLOT_C[15]),
        .I(SLOT_C_OUT[15]),
        .T(~SLOT_C_OUTEN[15]));
        
    IOBUF SLOT_C_16_IOBUF
       (.O(SLOT_C_IN[16]),
        .IO(SLOT_C[16]),
        .I(SLOT_C_OUT[16]),
        .T(~SLOT_C_OUTEN[16]));
        
    IOBUF SLOT_C_17_IOBUF
       (.O(SLOT_C_IN[17]),
        .IO(SLOT_C[17]),
        .I(SLOT_C_OUT[17]),
        .T(~SLOT_C_OUTEN[17]));
        
    IOBUF SLOT_C_18_IOBUF
       (.O(SLOT_C_IN[18]),
        .IO(SLOT_C[18]),
        .I(SLOT_C_OUT[18]),
        .T(~SLOT_C_OUTEN[18]));
        
    IOBUF SLOT_C_19_IOBUF
       (.O(SLOT_C_IN[19]),
        .IO(SLOT_C[19]),
        .I(SLOT_C_OUT[19]),
        .T(~SLOT_C_OUTEN[19]));
 
    IOBUF SLOT_C_20_IOBUF
       (.O(SLOT_C_IN[20]),
        .IO(SLOT_C[20]),
        .I(SLOT_C_OUT[20]),
        .T(~SLOT_C_OUTEN[20]));
        
    IOBUF SLOT_C_21_IOBUF
       (.O(SLOT_C_IN[21]),
        .IO(SLOT_C[21]),
        .I(SLOT_C_OUT[21]),
        .T(~SLOT_C_OUTEN[21]));


   IOBUF SLOT_D_0_IOBUF
       (.O(SLOT_D_IN[0]),
        .IO(SLOT_D[0]),
        .I(SLOT_D_OUT[0]),
        .T(~SLOT_D_OUTEN[0]));
        
    IOBUF SLOT_D_1_IOBUF
       (.O(SLOT_D_IN[1]),
        .IO(SLOT_D[1]),
        .I(SLOT_D_OUT[1]),
        .T(~SLOT_D_OUTEN[1]));
        
    IOBUF SLOT_D_2_IOBUF
       (.O(SLOT_D_IN[2]),
        .IO(SLOT_D[2]),
        .I(SLOT_D_OUT[2]),
        .T(~SLOT_D_OUTEN[2]));
        
    IOBUF SLOT_D_3_IOBUF
       (.O(SLOT_D_IN[3]),
        .IO(SLOT_D[3]),
        .I(SLOT_D_OUT[3]),
        .T(~SLOT_D_OUTEN[3]));
        
    IOBUF SLOT_D_4_IOBUF
       (.O(SLOT_D_IN[4]),
        .IO(SLOT_D[4]),
        .I(SLOT_D_OUT[4]),
        .T(~SLOT_D_OUTEN[4]));
        
    IOBUF SLOT_D_5_IOBUF
       (.O(SLOT_D_IN[5]),
        .IO(SLOT_D[5]),
        .I(SLOT_D_OUT[5]),
        .T(~SLOT_D_OUTEN[5]));
        
    IOBUF SLOT_D_6_IOBUF
       (.O(SLOT_D_IN[6]),
        .IO(SLOT_D[6]),
        .I(SLOT_D_OUT[6]),
        .T(~SLOT_D_OUTEN[6]));
        
    IOBUF SLOT_D_7_IOBUF
       (.O(SLOT_D_IN[7]),
        .IO(SLOT_D[7]),
        .I(SLOT_D_OUT[7]),
        .T(~SLOT_D_OUTEN[7]));
        
    IOBUF SLOT_D_8_IOBUF
       (.O(SLOT_D_IN[8]),
        .IO(SLOT_D[8]),
        .I(SLOT_D_OUT[8]),
        .T(~SLOT_D_OUTEN[8]));
        
    IOBUF SLOT_D_9_IOBUF
       (.O(SLOT_D_IN[9]),
        .IO(SLOT_D[9]),
        .I(SLOT_D_OUT[9]),
        .T(~SLOT_D_OUTEN[9]));
        
    IOBUF SLOT_D_10_IOBUF
       (.O(SLOT_D_IN[10]),
        .IO(SLOT_D[10]),
        .I(SLOT_D_OUT[10]),
        .T(~SLOT_D_OUTEN[10]));
        
    IOBUF SLOT_D_11_IOBUF
       (.O(SLOT_D_IN[11]),
        .IO(SLOT_D[11]),
        .I(SLOT_D_OUT[11]),
        .T(~SLOT_D_OUTEN[11]));
        
    IOBUF SLOT_D_12_IOBUF
       (.O(SLOT_D_IN[12]),
        .IO(SLOT_D[12]),
        .I(SLOT_D_OUT[12]),
        .T(~SLOT_D_OUTEN[12]));
        
    IOBUF SLOT_D_13_IOBUF
       (.O(SLOT_D_IN[13]),
        .IO(SLOT_D[13]),
        .I(SLOT_D_OUT[13]),
        .T(~SLOT_D_OUTEN[13]));
        
    IOBUF SLOT_D_14_IOBUF
       (.O(SLOT_D_IN[14]),
        .IO(SLOT_D[14]),
        .I(SLOT_D_OUT[14]),
        .T(~SLOT_D_OUTEN[14]));
        
    IOBUF SLOT_D_15_IOBUF
       (.O(SLOT_D_IN[15]),
        .IO(SLOT_D[15]),
        .I(SLOT_D_OUT[15]),
        .T(~SLOT_D_OUTEN[15]));
        
    IOBUF SLOT_D_16_IOBUF
       (.O(SLOT_D_IN[16]),
        .IO(SLOT_D[16]),
        .I(SLOT_D_OUT[16]),
        .T(~SLOT_D_OUTEN[16]));
        
    IOBUF SLOT_D_17_IOBUF
       (.O(SLOT_D_IN[17]),
        .IO(SLOT_D[17]),
        .I(SLOT_D_OUT[17]),
        .T(~SLOT_D_OUTEN[17]));
        
    IOBUF SLOT_D_18_IOBUF
       (.O(SLOT_D_IN[18]),
        .IO(SLOT_D[18]),
        .I(SLOT_D_OUT[18]),
        .T(~SLOT_D_OUTEN[18]));
        
    IOBUF SLOT_D_19_IOBUF
       (.O(SLOT_D_IN[19]),
        .IO(SLOT_D[19]),
        .I(SLOT_D_OUT[19]),
        .T(~SLOT_D_OUTEN[19]));
 
    IOBUF SLOT_D_20_IOBUF
       (.O(SLOT_D_IN[20]),
        .IO(SLOT_D[20]),
        .I(SLOT_D_OUT[20]),
        .T(~SLOT_D_OUTEN[20]));
        
    IOBUF SLOT_D_21_IOBUF
       (.O(SLOT_D_IN[21]),
        .IO(SLOT_D[21]),
        .I(SLOT_D_OUT[21]),
        .T(~SLOT_D_OUTEN[21]));
 
endmodule


