`timescale 1 ns/1 ps
module CcrfWrapper_tb();

reg CLK;
reg RESETN;

wire incoming_jobs_queue_populated;
wire jobs_to_schedule_queue_populated;
wire subtask_queue_populated;
wire completed_jobs_queue_populated;
wire ccrf_1_has_data;
wire ccrf_top_level_saw_data;
wire ccrf_top_level_scratchpad;
wire ccrf_subtask_scheduler_got_data;
wire ccrf_dispatcher_got_data;
wire incoming_jobs_populated_in_top_level;
wire [3:0] counter_out_V;
wire [3:0] counter_out_2_V;


wire response_message_queue_axi_V_TVALID;
wire [63 : 0] response_message_queue_axi_V_TDATA;
wire [3 : 0] response_message_queue_axi_V_TKEEP;
wire [3 : 0] response_message_queue_axi_V_TSTRB;
wire [0 : 0] response_message_queue_axi_V_TUSER;
wire [0 : 0] response_message_queue_axi_V_TLAST;
wire [0 : 0] response_message_queue_axi_V_TID;
wire [0 : 0] response_message_queue_axi_V_TDEST;

wire ccrf_top_level_saw_data_ap_vld;
wire ccrf_top_level_scratchpad_ap_vld;
wire incoming_job_request_from_top_level_populated;

reg incoming_job_requests_V_TVALID;
reg response_message_queue_axi_V_TREADY;
wire incoming_job_requests_V_TREADY; // output from dut
reg [575:0] incoming_job_requests_V_TDATA;
reg ap_start;
wire ap_ready; // output from dut
wire ap_done; // output from dut
wire ap_idle; // output from dut


reg [103:0] completed_jobs_queue_V_TDATA;
reg jobs_to_schedule_queue_V_TREADY;
reg completed_jobs_queue_V_TVALID;
wire jobs_to_schedule_queue_V_TVALID;
wire [495:0] jobs_to_schedule_queue_V_TDATA;



/* Add stimulus here */
always begin
#5 CLK = ~CLK;
end

initial begin
  $timeformat(-9,3,"ns",12);
  CLK = 0;
  RESETN = 0;
  completed_jobs_queue_V_TDATA = 104'b0;
  completed_jobs_queue_V_TVALID = 1'b0;
  incoming_job_requests_V_TDATA = {576{1'b1}};
  incoming_job_requests_V_TVALID = 1'b0;
  ap_start = 0;
  response_message_queue_axi_V_TREADY = 1'b0;
  jobs_to_schedule_queue_V_TREADY = 1'b0;
  

  #2
  response_message_queue_axi_V_TREADY = 1'b1;
  #2
  response_message_queue_axi_V_TREADY = 1'b0;
  #2
  response_message_queue_axi_V_TREADY = 1'b1;
  #2
  response_message_queue_axi_V_TREADY = 1'b0;
  #2
  response_message_queue_axi_V_TREADY = 1'b1;

  #100 
  RESETN = 1'b1;
  incoming_job_requests_V_TDATA = {576{1'b1}};
  response_message_queue_axi_V_TREADY = 1'b1;
  jobs_to_schedule_queue_V_TREADY = 1'b1;
  ap_start = 1'b1;
  
  #40
  // Send the CCRF_SCRATCHPAD_(START|END) addresses
  incoming_job_requests_V_TDATA = {576{1'b1}};
  incoming_job_requests_V_TDATA[519:512] = 8'd0; // job_ID
  //incoming_job_requests_V_TDATA[495:488] = 8'd0; // job_ID
  incoming_job_requests_V_TDATA[(16*8)-1:8*8] = 64'd1000000;
  incoming_job_requests_V_TDATA[(24*8)-1:16*8] = 64'd100000000;
  incoming_job_requests_V_TVALID = 1'b1;
  #10
  incoming_job_requests_V_TVALID = 1'b0;// Terminate the write
  incoming_job_requests_V_TDATA = {576{1'b1}};
  
  #40
  incoming_job_requests_V_TVALID = 1'b1;
  incoming_job_requests_V_TDATA = {576{1'b1}};
  incoming_job_requests_V_TDATA[519:512] = 8'd1; // Send an LDR image stack; jobID == 1
  //incoming_job_requests_V_TDATA[495:488] = 8'd1; // Send an LDR image stack; jobID == 1
  incoming_job_requests_V_TDATA[(8*8)-1:0*8] = 64'd100000; // OUTPUT
  incoming_job_requests_V_TDATA[(16*8)-1:8*8] = 64'd10000; // INPUT1
  incoming_job_requests_V_TDATA[(24*8)-1:16*8] = 64'd20000; // INPUT2
  incoming_job_requests_V_TDATA[(32*8)-1:24*8] = 64'd30000; // INPUT3
  incoming_job_requests_V_TDATA[(40*8)-1:32*8] = 64'd40000; // INPUT4
  incoming_job_requests_V_TDATA[(48*8)-1:40*8] = 64'd50000; // INPUT5
  incoming_job_requests_V_TDATA[(8*(8*7)) + 16 - 1: (8*(8*7))] = 16'd100; // IMAGE WIDTH
  incoming_job_requests_V_TDATA[(8*(8*7)) + 32 - 1: (8*(8*7)) + 16] = 16'd100; // IMAGE HEIGHT
  incoming_job_requests_V_TDATA[(8*(8*7)) + 40 - 1: (8*(8*7)) + 32] = 8'd5; // IMAGE COUNT
  response_message_queue_axi_V_TREADY = 1'b1;
  #10
  incoming_job_requests_V_TVALID = 1'b0;
  incoming_job_requests_V_TDATA = {576{1'b1}};
  response_message_queue_axi_V_TREADY = 1'b1;

  #230000 $finish;
end

/* Instantiation of top level design */

CcrfWrapperBlockDesign dut (
    .ccrf_top_level_saw_data(ccrf_top_level_saw_data),
    .ccrf_top_level_scratchpad(ccrf_top_level_scratchpad),
    .incoming_job_request_from_top_level_populated(incoming_job_request_from_top_level_populated),
    .incoming_job_requests_V_tvalid(incoming_job_requests_V_TVALID),
    .incoming_job_requests_V_tready(incoming_job_requests_V_TREADY),
    .incoming_job_requests_V_tdata(incoming_job_requests_V_TDATA),
    .response_message_queue_V_tvalid(response_message_queue_axi_V_TVALID),
    .response_message_queue_V_tready(response_message_queue_axi_V_TREADY),
    .response_message_queue_V_tdata(response_message_queue_axi_V_TDATA),
    .response_message_queue_V_tkeep(response_message_queue_axi_V_TKEEP),
    .response_message_queue_V_tstrb(response_message_queue_axi_V_TSTRB),
    .response_message_queue_V_tuser(response_message_queue_axi_V_TUSER),
    .response_message_queue_V_tlast(response_message_queue_axi_V_TLAST),
    .response_message_queue_V_tid(response_message_queue_axi_V_TID),
    .response_message_queue_V_tdest(response_message_queue_axi_V_TDEST),
    .aclk(CLK),
    .aresetn(RESETN)
    );


endmodule
