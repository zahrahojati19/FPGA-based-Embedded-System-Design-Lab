module TB();

    reg clk = 1'b0, rst = 1'b1, start = 1'b0;
    reg [11:0] N = 12'd10;
    reg [18:0] n_per_packet = 19'd50000;
    reg [31:0] mem_data = 32'd78;
    reg [31:0] base_addr = 32'd23;
    reg data_valid = 1'b1;
    reg ready = 1'b1;
    wire mem_read;
    wire [3:0] ps, ns;
    wire [31:0] write_data, addr;
    wire packet_ready, done;

    calc_mean UUT(clk, rst, start, N, n_per_packet, mem_data, base_addr, data_valid, ready, mem_read,
                  write_data, addr, packet_ready, done, ps, ns);

    always #5 clk = ~clk;
    initial begin
        #6 rst = 1'b0;
        #20 start = 1'b1;
        #73 ready = 1'b1;
        #2003; $stop();
    end
endmodule