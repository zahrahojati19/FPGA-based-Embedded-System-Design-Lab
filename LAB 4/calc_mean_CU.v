module calc_mean_CU (clk, rst, start, valid, co1, co2, ready, done, cnt1_rst, cnt2_rst, addr_rst,
                     cnt1_en, cnt2_en, reg_clr, reg_ld, addr_Inc, packet_ready, mem_read, ps, ns);
    input clk, rst, start, valid, co1, co2, ready;
    output reg done, cnt1_rst, cnt2_rst, addr_rst, cnt1_en, cnt2_en, reg_clr, reg_ld, addr_Inc, packet_ready, mem_read;

    output reg [3:0] ps, ns;
    parameter [3:0] idle = 0, counter_reset = 1, count1 = 2, wait_valid = 7, MEM_read = 3, calc = 4, write_data = 5, wait_ready = 8, Done = 6;

    always @(start, valid, co1, co2, ready, ps) begin
        ns = idle;
        case (ps)
            idle: ns = start ? counter_reset : idle;
            counter_reset: ns = count1;
            count1: ns = co1 ? Done : wait_valid;
            wait_valid: ns = valid ? MEM_read : wait_valid;
            MEM_read: ns = calc;
            calc: ns = co2 ? wait_ready : wait_valid;
            wait_ready: ns = ready ? write_data : wait_ready;
            write_data: ns = count1;
            Done: ns = idle;
        endcase
    end

    always @(ps) begin
        {done, cnt1_rst, cnt2_rst, addr_rst, cnt1_en, reg_clr, reg_ld, cnt2_en, addr_Inc, packet_ready, mem_read} = 11'b0;
        case (ps)
            idle: ;
            counter_reset: {cnt1_rst, cnt2_rst, addr_rst} = 3'b111;
            count1: reg_clr = 1'b1;
            wait_valid: mem_read = 1'b1;
            MEM_read: cnt2_en = 1'b1;
            calc: {addr_Inc, reg_ld} = 2'b11;
            wait_ready: packet_ready = 1'b1;
            write_data: cnt1_en = 1'b1;
            Done: done = 1'b1;
        endcase
    end 

    always @(posedge clk, posedge rst)begin
        if(rst)
            ps <= idle;
        else
            ps <= ns;
    end  
endmodule