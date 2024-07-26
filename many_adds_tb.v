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
   wire [4:0] raddr2;   

   wire [31:0] rdata0;
   wire [31:0] rdata1;   
   wire [31:0] rdata2;   

   initial begin

      #1 dbg_wr_addr = 0;
      #1 dbg_wr_data = 1;
      #1 dbg_wr_en = 1;
      
      #1 clk = 0;
      #1 clk = 1;
      #1 rst = 1;
      
      #1 dbg_wr_addr = 1;
      #1 dbg_wr_data = 2;
      #1 dbg_wr_en = 1;
      
      #1 clk = 0;
      #1 clk = 1;
      #1 rst = 1;

      #1 dbg_wr_addr = 2;
      #1 dbg_wr_data = 3;
      #1 dbg_wr_en = 1;
      
      #1 clk = 0;
      #1 clk = 1;
      #1 rst = 1;
      
      #1 dbg_wr_en = 0;
      
      #1 clk = 0;
      #1 clk = 1;
      #1 rst = 1;
      
      #1 clk = 0;
      #1 clk = 1;
      #1 rst = 1;
      
      #1 dbg_addr = 3;
      
      #1 clk = 0;
      #1 rst = 1;
      #1 clk = 1;

      $display("dbg data = %d", dbg_data);

//      `assert(dbg_data, 32'hxxxxxxxx)
      `assert(valid, 1'd0)
      
      //$display("rdata = %d", rdata);
      // $display("wen   = %d", wen);
      // $display("waddr = %d", waddr);
      // $display("wdata = %d", wdata);
      #1 rst = 0;

      #1 clk = 0;
      #1 clk = 1;

      #1 clk = 0;
      #1 clk = 1;

      `assert(valid, 1'd0)                  
//      #1 `assert(rdata0, 32'hxxxxxxxx)

      #1 clk = 0;
      #1 clk = 1;

      $display("dbg data = %d", dbg_data);            
//      $display("rdata = %d", rdata);      
      `assert(valid, 1'd0)            

      $display("dbg data = %d", dbg_data);            
      
      #1 clk = 0;
      #1 clk = 1;

      $display("dbg data = %d", dbg_data);            

      #1 clk = 0;
      #1 clk = 1;

      $display("dbg data = %d", dbg_data);

      #1 clk = 0;
      #1 clk = 1;

      $display("dbg data = %d", dbg_data);

      #1 clk = 0;
      #1 clk = 1;
      
      $display("dbg data = %d", dbg_data);
      #1 `assert(dbg_data, 32'd6)
      #1 `assert(valid, 1'd1)      
      
      #1 $display("Passed");

   end

   RAM3 mem(.clk(clk),
            .rst(rst),

            .raddr_0(raddr0),
            .rdata_0(rdata0),

            .raddr_1(raddr1),
            .rdata_1(rdata1),

            .raddr_2(raddr2),
            .rdata_2(rdata2),
            
            .debug_write_addr(dbg_wr_addr),
            .debug_write_data(dbg_wr_data),
            .debug_write_en(dbg_wr_en),
            
            .debug_addr(dbg_addr),
            .debug_data(dbg_data),
            
            .wen_0(wen),
            .wdata_0(wdata),
            .waddr_0(waddr));
   
   many_adds ss(.clk(clk), .rst(rst),
                .valid(valid),
                .ram_waddr_0(waddr), .ram_wdata_0(wdata), .ram_wen_0(wen),
                .ram_raddr_0(raddr0), .ram_rdata_0(rdata0),
                .ram_raddr_1(raddr1), .ram_rdata_1(rdata1),
                .ram_raddr_2(raddr2), .ram_rdata_2(rdata2));
   
endmodule
