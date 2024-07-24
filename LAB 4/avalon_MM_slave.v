module AVS_AVALONSLAVE #
(
  parameter integer AVS_AVALONSLAVE_DATA_WIDTH = 32,
  parameter integer AVS_AVALONSLAVE_ADDRESS_WIDTH = 1
)
(
	 // user ports begin  
	 //(ports to comminucate with the main module) 
	 
	 // user ports end
	 
  // dont change these ports
  input wire CSI_CLOCK_CLK,
  input wire CSI_CLOCK_RESET_N,
  input wire [AVS_AVALONSLAVE_ADDRESS_WIDTH - 1:0] AVS_AVALONSLAVE_ADDRESS,
  input wire AVS_AVALONSLAVE_READ,
  input wire AVS_AVALONSLAVE_WRITE,
  input done,
  output reg [AVS_AVALONSLAVE_DATA_WIDTH - 1:0] AVS_AVALONSLAVE_READDATA,
  input wire [AVS_AVALONSLAVE_DATA_WIDTH - 1:0] AVS_AVALONSLAVE_WRITEDATA,
  output Go,
  output [11:0] N,
  output [18:0] n_per_packet,
  output [31:0] base_addr
);
  
  reg [AVS_AVALONSLAVE_DATA_WIDTH - 1:0] slv_reg0;
  reg [AVS_AVALONSLAVE_DATA_WIDTH - 1:0] slv_reg1;

  always @(posedge CSI_CLOCK_CLK)
  begin
    if(CSI_CLOCK_RESET_N == 0)
    begin
      slv_reg0 <= 0;
      slv_reg1 <= 0;
    end
    else if(done)
      slv_reg0[31] = done;
    else if(AVS_AVALONSLAVE_WRITE)
    begin
      case(AVS_AVALONSLAVE_ADDRESS)
      0: slv_reg0 <= AVS_AVALONSLAVE_WRITEDATA;
      1: slv_reg1 <= AVS_AVALONSLAVE_WRITEDATA;
      endcase
    end
  end
 always @(*)begin
    case(AVS_AVALONSLAVE_ADDRESS)
      0: AVS_AVALONSLAVE_READDATA <= slv_reg0;
      1: AVS_AVALONSLAVE_READDATA <= slv_reg1;
      default: AVS_AVALONSLAVE_READDATA <= slv_reg0;
    endcase
  end  
	//user code begin
	assign Go = slv_reg0[0];
  assign N = slv_reg0[11:1];
  assign n_per_packet = slv_reg0[30:12];
  assign base_addr = slv_reg1;
	//user code end
endmodule

