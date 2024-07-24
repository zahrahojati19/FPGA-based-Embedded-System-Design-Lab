`timescale 1ns/1ns
module MEM_TB();

    reg clk = 1'b0, rst = 1'b1, start = 1'b0;
    reg [11:0] N = 12'd10;
    reg [18:0] n_per_packet = 19'd100;
    reg [31:0] base_addr = 32'd0;
    reg ready = 1'b1;

    wire data_valid, mem_read, packet_ready, done;
    wire [31:0] mem_data, write_data, addr, Out;
    wire [3:0] ps, ns;

    calc_mean Calc_Mean(clk, rst, start, N, n_per_packet, mem_data, base_addr, data_valid, ready, mem_read,
                  write_data, addr, packet_ready, done, ps, ns);
    SDRAM mem(clk, rst, addr, mem_read, data_valid, mem_data);
    shiftreg Shift_Reg(clk, rst, packet_ready, write_data, Out);

    always #5 clk = ~clk;
    initial begin
        #6 rst = 1'b0;
        #20 start = 1'b1;
        #73 ready = 1'b1;
        #100000 $stop;
    end
endmodule