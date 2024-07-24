module calc_mean_DP (clk, rst, N, n_per_packet, mem_data, reg_clr, reg_ld, addr_Inc, addr_rst, base_addr,
                     cnt1_rst, cnt1_en, cnt2_rst, cnt2_en, co1, co2, addr, write_data);
    input clk, rst, reg_clr, reg_ld, addr_Inc, addr_rst, cnt1_rst, cnt1_en, cnt2_rst, cnt2_en;
    input [11:0] N;
    input [18:0] n_per_packet;
    input [31:0] mem_data, base_addr; 
    output co1, co2;
    output reg [31:0] addr; 
    output reg [31:0] write_data;

    reg [11:0] count1;
    reg [18:0] count2;
    reg [31:0] reg_out;
    wire [31:0] sum;

    always @(posedge clk, posedge rst) begin
        if(rst)
            count1 <= 32'b0;
        else if(cnt1_rst | co1)
            count1 <= 32'b0;
        else if(cnt1_en)
            count1 <= count1 + 1'b1;
    end
    assign co1 = (count1 == N) ? 1'b1: 1'b0;
    
    always @(posedge clk, posedge rst) begin
        if(rst)
            count2 <= 32'b0;
        else if(cnt2_rst | co2)
            count2 <= 32'b0;
        else if(cnt2_en)
            count2 <= count2 + 1'b1;
    end
    assign co2 = (count2 == n_per_packet) ? 1'b1: 1'b0;
    
    always @(posedge clk, posedge rst) begin
        if(rst)
            addr <= 32'b0;
        else if(addr_rst)
            addr <= 32'b0;
        else if(addr_Inc)
            addr <= addr + 32'd4;
    end
    assign sum = mem_data + reg_out;

    always @(posedge clk, posedge rst) begin
        if(rst)
            reg_out = 32'b0;
        else if(reg_clr)
            reg_out = 32'b0;
        else if(reg_ld)
            reg_out = sum;
    end
wire x;
assign x = (reg_out / n_per_packet);
    always @(posedge clk)begin
	//sum = mem_data + reg_out;
        write_data = (reg_out / n_per_packet);//+1'b1;
    end
    
endmodule