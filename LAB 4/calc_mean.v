module calc_mean (clk, rst, start, N, n_per_packet, mem_data, base_addr, data_valid, ready, mem_read,
                  write_data, addr, packet_ready, done, ps, ns);
    input clk, rst, start, data_valid, ready;
    input [11:0] N;
    input [18:0] n_per_packet;
    input [31:0] mem_data, base_addr;

    output mem_read, packet_ready, done;
    output [31:0] write_data, addr;

    output [3:0] ps, ns;

    wire cnt1_rst, cnt2_rst, addr_rst, cnt1_en, reg_clr, reg_ld, cnt2_en, addr_Inc, co1, co2;

    calc_mean_DP DP(clk, rst, N, n_per_packet, mem_data, reg_clr, reg_ld, addr_Inc, addr_rst,
                    base_addr, cnt1_rst, cnt1_en, cnt2_rst, cnt2_en, co1, co2, addr, write_data);

    calc_mean_CU CU(clk, rst, start, data_valid, co1, co2, ready, done, cnt1_rst, cnt2_rst, addr_rst,
                    cnt1_en, cnt2_en, reg_clr, reg_ld, addr_Inc, packet_ready, mem_read, ps, ns);
endmodule