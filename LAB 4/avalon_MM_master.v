
module AVM_AVALONMASTER #
(
  parameter integer AVM_AVALONMASTER_DATA_WIDTH = 32,
  parameter integer AVM_AVALONMASTER_ADDRESS_WIDTH = 32
)
(
  // user ports begin

  // user ports end
  
  // dont change these ports
  input wire CSI_CLOCK_CLK,
  input wire CSI_CLOCK_RESET_N,
  output wire [AVM_AVALONMASTER_ADDRESS_WIDTH - 1:0] AVM_AVALONMASTER_ADDRESS,
  input wire AVM_AVALONMASTER_WAITREQUEST,
  output wire AVM_AVALONMASTER_READ,
  input wire [AVM_AVALONMASTER_DATA_WIDTH - 1:0] AVM_AVALONMASTER_READDATA
  );

  reg [AVM_AVALONMASTER_ADDRESS_WIDTH - 1:0] address;
  reg read;

  // I/O assignment
  
  // never directly send values to output
  assign AVM_AVALONMASTER_ADDRESS = address;
  assign AVM_AVALONMASTER_READ = read;


  // user logic begin

  // user logic end

endmodule

