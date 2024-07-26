`define assert(signal, value) if ((signal) !== (value)) begin $display("ASSERTION FAILED in %m: signal != value"); $finish(1); end

module test();

   reg clk;
   reg rst;
   wire valid;
   

   // Depth 16, width 32 RAM
   reg [4:0] dbg_addr;
   wire [31:0] dbg_data;

   reg [4:0] dbg_wr_addr;   
   reg [31:0] dbg_wr_data;
   reg dbg_wr_en;   

   wire [4:0] waddr;
   wire [31:0] wdata;
   wire [0:0] wen;

   wire [4:0] raddr0;
   wire [4:0] raddr1;
   
   wire [31:0] rdata0;
   wire [31:0] rdata1;   

   initial begin

      #1 dbg_wr_addr = 2; // a + 2
      #1 dbg_wr_data = 34;
      #1 dbg_wr_en = 1;

      #1 rst = 1;
      
      #1 clk = 0;
      #1 clk = 1;

      // $display("dbg_data    = %d", dbg_data);
      // $display("raddr0      = %d", raddr0);

      #1 rst = 1;
      
       #1 dbg_wr_en = 0;
      
      #1 clk = 0;
      #1 clk = 1;

      // $display("dbg_data = %d", dbg_data);
      // $display("raddr0   = %d", raddr0);

      #1 rst = 1;
      
      #1 dbg_addr = 3;

      //`assert(32'd1, 32'd0)
      
      #1 rst = 0;

      #1 clk = 0;
      #1 clk = 1;

      // $display("dbg_data = %d", dbg_data);
      // $display("raddr0   = %d", raddr0);

      #1 clk = 0;
      #1 clk = 1;

      // $display("dbg_data = %d", dbg_data);
      // $display("raddr0   = %d", raddr0);
      #1 clk = 0;
      #1 clk = 1;

      // $display("dbg_data = %d", dbg_data);
      // $display("raddr0   = %d", raddr0);
      #1 clk = 0;
      #1 clk = 1;

      // $display("dbg_data = %d", dbg_data);
      // $display("raddr0   = %d", raddr0);
      #1 clk = 0;
      #1 clk = 1;

      // $display("dbg_data = %d", dbg_data);
      // $display("raddr0   = %d", raddr0);
      #1 clk = 0;
      #1 clk = 1;

      // $display("dbg_data = %d", dbg_data);
      // $display("raddr0   = %d", raddr0);

      `assert(dbg_data, 32'd34)
      `assert(valid, 1'd1)      

      #1 $display("Passed");

   end

   RAM2 mem(.clk(clk),
            .rst(rst),

            .raddr_0(raddr0),
            .rdata_0(rdata0),

            .raddr_1(raddr1),
            .rdata_1(rdata1),

            .debug_write_addr(dbg_wr_addr),
            .debug_write_data(dbg_wr_data),
            .debug_write_en(dbg_wr_en),
            
            .debug_addr(dbg_addr),
            .debug_data(dbg_data),
            
            .wen(wen),
            .wdata(wdata),
            .waddr(waddr));
   
   read_2 ss(.clk(clk), .rst(rst), .valid(valid), .mem_waddr_0(waddr), .mem_wdata_0(wdata), .mem_wen_0(wen), .mem_raddr_0(raddr0), .mem_rdata_0(rdata0));
   
endmodule
