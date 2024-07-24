module ACCELERATOR_CTRL #
(
  // you can add parameters here
  // you can change these parameters

  // control interface parameters
  parameter integer avs_avalonslave_data_width = 32,
  parameter integer avs_avalonslave_address_width = 1,

  // control interface parameters
  parameter integer avm_avalonmaster_data_width = 32,
  parameter integer avm_avalonmaster_address_width = 32,
  
  // streaming source interface parameters
  parameter AVALONSOURCE_DATA_WIDTH = 32
)
(
  // user ports begin

  // user ports end
  
  // dont change these ports
  // clock and reset
  input wire csi_clock_clk,
  input wire csi_clock_reset_n,

  // control interface ports
  input wire [avs_avalonslave_address_width - 1:0] avs_avalonslave_address,
  output wire avs_avalonslave_waitrequest,
  input wire avs_avalonslave_read,
  input wire avs_avalonslave_write,
  output wire [avs_avalonslave_data_width - 1:0] avs_avalonslave_readdata,
  input wire [avs_avalonslave_data_width - 1:0] avs_avalonslave_writedata,

  // master interface ports
  output wire [avm_avalonmaster_address_width - 1:0] avm_avalonmaster_address,
  input wire avm_avalonmaster_waitrequest,
  output wire avm_avalonmaster_read,
  input wire [avm_avalonmaster_data_width - 1:0] avm_avalonmaster_readdata,

  //streaming source interface ports
  input wire aso_avalonst_ready,
  output wire aso_avalonst_valid,
  output wire [AVALONSOURCE_DATA_WIDTH-1:0] aso_avalonst_data,
  output wire aso_avalonst_startofpacket,
  output wire aso_avalonst_endofpacket,
  // input wire ready,
  // output wire valid,
  // output wire [AVALONSOURCE_DATA_WIDTH-1:0] data,
  // output wire start_of_packet,
  // output wire end_of_packet
  output wire [3:0] ps, ns
);

// define your extra ports as wire here
wire Go, done;
wire [11:0] N;
wire [18:0] n_per_packet;
wire [31:0] base_addr;
// control interface instanciation
AVS_AVALONSLAVE #
(
  // you can add parameters here
  // you can change these parameters
  .AVS_AVALONSLAVE_DATA_WIDTH(avs_avalonslave_data_width),
  .AVS_AVALONSLAVE_ADDRESS_WIDTH(avs_avalonslave_address_width)
) AVS_AVALONSLAVE_INST // instance  of module must be here
(
  // user ports begin
  
  // user ports end
  // dont change these ports
  .CSI_CLOCK_CLK(csi_clock_clk),
  .CSI_CLOCK_RESET_N(csi_clock_reset_n),
  .AVS_AVALONSLAVE_ADDRESS(avs_avalonslave_address),
  .AVS_AVALONSLAVE_READ(avs_avalonslave_read),
  .AVS_AVALONSLAVE_WRITE(avs_avalonslave_write),
  .AVS_AVALONSLAVE_READDATA(avs_avalonslave_readdata),
  .AVS_AVALONSLAVE_WRITEDATA(avs_avalonslave_writedata),
  .done(done),
  .Go(Go),
  .N(N),
  .n_per_packet(n_per_packet),
  .base_addr(base_addr)
);

// magnitude interface instanciation
// AVM_AVALONMASTER_MAGNITUDE #
// (
//   // you can add parameters here
//   // you can change these parameters
//   .AVM_AVALONMASTER_DATA_WIDTH(avm_avalonmaster_data_width),
//   .AVM_AVALONMASTER_ADDRESS_WIDTH(avm_avalonmaster_address_width)
// ) AVM_AVALONMASTER_MAGNITUDE_INST // instance  of module must be here
// (
//   // user ports begin
//   // user ports end
//   // dont change these ports
//   .CSI_CLOCK_CLK(csi_clock_clk),
//   .CSI_CLOCK_RESET_N(csi_clock_reset_n),
//   .AVM_AVALONMASTER_ADDRESS(avm_avalonmaster_address),
//   .AVM_AVALONMASTER_WAITREQUEST(avm_avalonmaster_waitrequest),
//   .AVM_AVALONMASTER_READ(avm_avalonmaster_read),
//   .AVM_AVALONMASTER_READDATA(avm_avalonmaster_readdata)
// );

// AVALONSOURCE #
// (
//   .DATA_WIDTH(AVALONSOURCE_DATA_WIDTH)
// )
// AVALONSOURCE_INST(
//  //user ports begin                                                            
//  //user ports end
//   .clk(csi_clock_clk),
//   .rst_n(csi_clock_reset_n),
//   .ready(ready),
//   .valid(valid),
//   .data(data),
//   .start_of_packet(start_of_packet),
//   .end_of_packet(end_of_packet)
// );

calc_mean(
    .clk(csi_clock_clk),
    .rst(!csi_clock_reset_n), .start(Go),
    .N(N),
    .n_per_packet(n_per_packet),
    .mem_data(avm_avalonmaster_readdata),
    .base_addr(base_addr), 
    .data_valid(!avm_avalonmaster_waitrequest),
    .ready(aso_avalonst_ready),
    .mem_read(avm_avalonmaster_read), 
    .write_data(aso_avalonst_data), .addr(avm_avalonmaster_address),
    .packet_ready(aso_avalonst_valid), .done(done), .ps(ps), .ns(ns)
);
  assign aso_avalonst_startofpacket = 1'b0;
  assign aso_avalonst_endofpacket = 1'b0;
  assign avs_avalonslave_waitrequest = 1'b0;
endmodule