’timescale 2 ns/1 ps
module CcrfWrapper_tb;
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

reg incoming_job_requests_V_TVALID;
reg response_message_queue_axi_V_TREADY;
wire incoming_job_requests_V_TREADY; // output from dut
reg [495:0] incoming_job_requests_V_TDATA;
reg ap_start;
wire ap_ready; // output from dut
wire ap_done; // output from dut
wire ap_idle; // output from dut


/* Instantiation of top level design */
module design_1_CcrfWrapper_0_3 (
  .incoming_jobs_queue_populated(incoming_jobs_queue_populated),
  .jobs_to_schedule_queue_populated(jobs_to_schedule_queue_populated),
  .subtask_queue_populated(subtask_queue_populated),
  .jobs_in_progress_queue_populated(jobs_in_progress_queue_populated),
  .completed_jobs_queue_populated(completed_jobs_queue_populated),
  .ccrf_1_has_data(ccrf_1_has_data),
  .ccrf_top_level_saw_data(ccrf_top_level_saw_data),
  .ccrf_top_level_scratchpad(ccrf_top_level_scratchpad),
  .ccrf_subtask_scheduler_got_data(ccrf_subtask_scheduler_got_data),
  .ccrf_dispatcher_got_data(ccrf_dispatcher_got_data),
  .incoming_jobs_populated_in_top_level(incoming_jobs_populated_in_top_level),
  .counter_out_V(counter_out_V),
  .counter_out_2_V(counter_out_2_V),
  .incoming_job_requests_V_TVALID(incoming_job_requests_V_TVALID),
  .incoming_job_requests_V_TREADY(incoming_job_requests_V_TREADY),
  .incoming_job_requests_V_TDATA(incoming_job_requests_V_TDATA),
  .response_message_queue_axi_V_TVALID(response_message_queue_axi_V_TVALID),
  .response_message_queue_axi_V_TREADY(response_message_queue_axi_V_TREADY),
  .response_message_queue_axi_V_TDATA(response_message_queue_axi_V_TDATA),
  .response_message_queue_axi_V_TKEEP(response_message_queue_axi_V_TKEEP),
  .response_message_queue_axi_V_TSTRB(response_message_queue_axi_V_TSTRB),
  .response_message_queue_axi_V_TUSER(response_message_queue_axi_V_TUSER),
  .response_message_queue_axi_V_TLAST(response_message_queue_axi_V_TLAST),
  .response_message_queue_axi_V_TID(response_message_queue_axi_V_TID),
  .response_message_queue_axi_V_TDEST(response_message_queue_axi_V_TDEST),
  .ap_start(ap_start),
  .ap_ready(ap_ready),
  .ap_done(ap_done),
  .ap_idle(ap_idle),
  .aclk(CLK),
  .areset(RESETN)
);

/* Add stimulus here */
always #10 CLK = ~CLK;
initial begin
$timeformat(-9,3,"ns",12);
end
initial begin
RESETN = 0
C_INT = 0;

incoming_job_requests_V_TVALID = 0;
incoming_job_requests_V_TDATA = 0;
ap_start = 0;
response_message_queue_axi_V_TREADY = 0;

AT = 0;
BT = 0;
CLK = 1;
#100 
RESETN = 1'b1;
response_message_queue_axi_V_TREADY = 1'b1;
#100
incoming_job_requests_V_TVALID = 1'b1;
incoming_job_requests_V_TDATA = 496'd99;
#10
incoming_job_requests_V_TVALID = 1'b0;
incoming_job_requests_V_TDATA = 496'b0;

#1000 $finish;
end


/* end stimulus section */