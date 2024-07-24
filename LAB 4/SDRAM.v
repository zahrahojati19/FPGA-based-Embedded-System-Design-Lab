module SDRAM(clk, rst, addr, mem_read, data_valid, mem_data);
    input clk, rst, mem_read;
    input [31:0] addr;
    output data_valid;
    output reg [31:0] mem_data;

    reg [31:0] mem [499999:0];
    integer i;

    always @(posedge clk)begin
        if(rst)
            for(i = 0; i < 500000; i = i + 1)
                mem[i] <= i + 1;
        else if(mem_read)
            mem_data <= mem[addr/4];
    end
    assign data_valid = 1'b1;
endmodule