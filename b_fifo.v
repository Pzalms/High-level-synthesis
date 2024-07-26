module b_fifo(input clk,
            input                  rst,

            input                  read_valid,
            output                 read_ready,

            input                  write_valid,
            output                 write_ready,
            
            input [WIDTH - 1 : 0]  in_data,
            output [WIDTH - 1 : 0] out_data);
   
   parameter WIDTH = 32;
   parameter DEPTH = 16;

   reg [WIDTH - 1 : 0]               ram [DEPTH - 1 : 0];

   reg                               empty;

   reg [$clog2(DEPTH) - 1 : 0]                write_addr;
   reg [$clog2(DEPTH) - 1 : 0]                read_addr;
   wire [$clog2(DEPTH) - 1 : 0]                next_read_addr;
   wire [$clog2(DEPTH) - 1 : 0]                next_write_addr;

   always @(posedge clk) begin
      if (!rst) begin
      end
   end


   
   assign next_read_addr = (DEPTH == (read_addr + 1)) ? 0 : read_addr + 1;
   assign next_write_addr = (DEPTH == (write_addr + 1)) ? 0 : write_addr + 1;

   reg [WIDTH - 1 : 0] out_data_reg;

   always @(posedge clk) begin
      if (read_valid) begin
         out_data_reg <= ram[read_addr];
      end
   end

   assign out_data = out_data_reg;
   
   assign full = !empty && (write_addr == read_addr);
   assign write_ready = !full;

   // always @(posedge clk) begin
   //    $display("empty = %d", empty);
   //    $display("full  = %d", full);      
   // end

   assign read_ready = !empty;

   always @(posedge clk) begin
      if (rst) begin
         empty <= 1;

//         $display("reseting");
         write_addr <= 0;
         read_addr <= 0;
      end else begin
         if (read_valid) begin

            read_addr <= next_read_addr;

            if (!empty && (next_read_addr == write_addr) && !write_valid) begin
               empty <= 1;
            end
         end // if (read_valid)

         if (write_valid) begin

            ram[write_addr] <= in_data;
            write_addr <= next_write_addr;

            empty <= 0;
         end
         
      end
   end
   
endmodule
