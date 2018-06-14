`timescale 1 ns/1 ps
module CcrfWrapper_tb();

reg CLK;
reg RESETN;

wire incoming_jobs_queue_populated;
wire jobs_to_schedule_queue_populated;
wire subtask_queue_populated;
wire jobs_in_progress_queue_populated;
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
wire [31 : 0] response_message_queue_axi_V_TDATA;
wire [3 : 0] response_message_queue_axi_V_TKEEP;
wire [3 : 0] response_message_queue_axi_V_TSTRB;
wire [0 : 0] response_message_queue_axi_V_TUSER;
wire [0 : 0] response_message_queue_axi_V_TLAST;
wire [0 : 0] response_message_queue_axi_V_TID;
wire [0 : 0] response_message_queue_axi_V_TDEST;

wire ccrf_top_level_saw_data_ap_vld;
wire ccrf_top_level_scratchpad_ap_vld;
wire incoming_job_request_from_top_level_populated;
wire incoming_job_request_from_top_level_populated_ap_vld;

reg incoming_job_requests_V_TVALID;
reg response_message_queue_axi_V_TREADY;
wire incoming_job_requests_V_TREADY; // output from dut
reg [495:0] incoming_job_requests_V_TDATA;
reg ap_start;
wire ap_ready; // output from dut
wire ap_done; // output from dut
wire ap_idle; // output from dut


wire completed_jobs_queue_V_TREADY;
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
  incoming_job_requests_V_TDATA = 496'd0;
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
  incoming_job_requests_V_TDATA = 496'd0;
  response_message_queue_axi_V_TREADY = 1'b1;
  jobs_to_schedule_queue_V_TREADY = 1'b1;
  ap_start = 1'b1;
  
  #40
  incoming_job_requests_V_TVALID = 1'b1;
  incoming_job_requests_V_TDATA[487:0] = 488'd99;
  incoming_job_requests_V_TDATA[495:488] = 8'd1;
  response_message_queue_axi_V_TREADY = 1'b1;
  #10
  incoming_job_requests_V_TVALID = 1'b0;
  incoming_job_requests_V_TDATA = 496'b0;
  response_message_queue_axi_V_TREADY = 1'b1;

  #1000 $finish;
end

/* Instantiation of top level design */
CcrfTopLevel dut (
    .ccrf_top_level_saw_data(ccrf_top_level_saw_data),
    .ccrf_top_level_saw_data_ap_vld(ccrf_top_level_saw_data_ap_vld),
    .ccrf_top_level_scratchpad(ccrf_top_level_scratchpad),
    .ccrf_top_level_scratchpad_ap_vld(ccrf_top_level_scratchpad_ap_vld),
    .incoming_job_request_from_top_level_populated(incoming_job_request_from_top_level_populated),
    .incoming_job_request_from_top_level_populated_ap_vld(incoming_job_request_from_top_level_populated_ap_vld),
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

/*
CcrfTopLevel dut (
    .ccrf_top_level_saw_data(ccrf_top_level_saw_data),
    .ccrf_top_level_saw_data_ap_vld(ccrf_top_level_saw_data_ap_vld),
    .ccrf_top_level_scratchpad(ccrf_top_level_scratchpad),
    .ccrf_top_level_scratchpad_ap_vld(ccrf_top_level_scratchpad_ap_vld),
    .incoming_job_request_from_top_level_populated(incoming_job_request_from_top_level_populated),
    .incoming_job_request_from_top_level_populated_ap_vld(incoming_job_request_from_top_level_populated_ap_vld),
    .completed_jobs_queue_V_TVALID(completed_jobs_queue_V_TVALID),
    .completed_jobs_queue_V_TREADY(completed_jobs_queue_V_TREADY),
    .completed_jobs_queue_V_TDATA(completed_jobs_queue_V_TDATA),
    .incoming_job_requests_V_TVALID(incoming_job_requests_V_TVALID),
    .incoming_job_requests_V_TREADY(incoming_job_requests_V_TREADY),
    .incoming_job_requests_V_TDATA(incoming_job_requests_V_TDATA),
    .jobs_to_schedule_queue_V_TVALID(jobs_to_schedule_queue_V_TVALID),
    .jobs_to_schedule_queue_V_TREADY(jobs_to_schedule_queue_V_TREADY),
    .jobs_to_schedule_queue_V_TDATA(jobs_to_schedule_queue_V_TDATA),
    .response_message_queue_V_TVALID(response_message_queue_axi_V_TVALID),
    .response_message_queue_V_TREADY(response_message_queue_axi_V_TREADY),
    .response_message_queue_V_TDATA(response_message_queue_axi_V_TDATA),
    .response_message_queue_V_TKEEP(response_message_queue_axi_V_TKEEP),
    .response_message_queue_V_TSTRB(response_message_queue_axi_V_TSTRB),
    .response_message_queue_V_TUSER(response_message_queue_axi_V_TUSER),
    .response_message_queue_V_TLAST(response_message_queue_axi_V_TLAST),
    .response_message_queue_V_TID(response_message_queue_axi_V_TID),
    .response_message_queue_V_TDEST(response_message_queue_axi_V_TDEST),
    .aclk(CLK),
    .aresetn(RESETN)
    );
*/


endmodule
