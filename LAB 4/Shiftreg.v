module shiftreg(clk, rst, sh_ld, In, Out);
    input clk, rst, sh_ld;
    input [31:0] In;
    output reg [31:0] Out;

    reg [31:0] A, B, C;

    always @(posedge clk, posedge rst)begin
        if(rst)
            {A, B, C, Out} = 128'd0;
        else if(sh_ld)begin
            A <= In;
            B <= A;
            C <= B;
            Out <= C;
        end
    end

endmodule