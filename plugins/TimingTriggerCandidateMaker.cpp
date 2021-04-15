#include "TimingTriggerCandidateMaker.hpp"

namespace dunedaq {
namespace trigger {
TimingTriggerCandidateMaker::TimingTriggerCandidateMaker(const std::string& name)
  : DAQModule(name)
  , thread_(std::bind(&TimingTriggerCandidateMaker::do_work, this, std::placeholders::_1))
  , inputQueue_(nullptr)
  , outputQueue_(nullptr)
  , queueTimeout_(100)
{

  register_command("conf", &TimingTriggerCandidateMaker::do_conf);
  register_command("start", &TimingTriggerCandidateMaker::do_start);
  register_command("stop", &TimingTriggerCandidateMaker::do_stop);
  register_command("scrap", &TimingTriggerCandidateMaker::do_scrap);
}

triggeralgs::TriggerCandidate
TimingTriggerCandidateMaker::TimeStampedDataToTriggerCandidate(const triggeralgs::TimeStampedData& data)
{
  triggeralgs::TriggerCandidate candidate;
  try {
	  candidate.time_start = data.time_stamp - m_detid_offsets_map[data.signal_type].first;  // time_start
	  candidate.time_end = data.time_stamp + m_detid_offsets_map[data.signal_type].second; // time_end,
  } catch (const ers::Issue& excpt) {
    throw dunedaq::trigger::SignalTypeError(ERS_HERE, get_name(), data.signal_type, excpt);
  }
  candidate.time_candidate = data.time_stamp;
  candidate.detid = {static_cast<uint16_t>(data.signal_type)};
  candidate.type = TriggerCandidateType::kTiming;
  candidate.algorithm = 0;
  candidate.version = 0;
  candidate.ta_list = {};

  return candidate;
}

void
TimingTriggerCandidateMaker::do_conf(const nlohmann::json& config)
{
  auto params = config.get<dunedaq::trigger::timingtriggercandidatemaker::Conf>();
  m_detid_offsets_map[ params.s0.signal_type ] = { params.s0.time_before, params.s0.time_after };
  m_detid_offsets_map[ params.s1.signal_type ] = { params.s1.time_before, params.s1.time_after };
  m_detid_offsets_map[ params.s2.signal_type ] = { params.s2.time_before, params.s2.time_after };
  TLOG_DEBUG(2) << get_name() + " configured.";
}

void
TimingTriggerCandidateMaker::init(const nlohmann::json& iniobj)
{
  try {
    auto qi = appfwk::queue_index(iniobj, { "input", "output" });
    inputQueue_.reset(new source_t(qi["input"].inst));
    outputQueue_.reset(new sink_t(qi["output"].inst));
  } catch (const ers::Issue& excpt) {
    throw dunedaq::trigger::InvalidQueueFatalError(ERS_HERE, get_name(), "input/output", excpt);
  }
}

void
TimingTriggerCandidateMaker::do_start(const nlohmann::json&)
{
  thread_.start_working_thread();
  TLOG_DEBUG(2) << get_name() + " successfully started.";
}

void
TimingTriggerCandidateMaker::do_stop(const nlohmann::json&)
{
  thread_.stop_working_thread();
  TLOG_DEBUG(2) << get_name() + " successfully stopped.";
}

void
TimingTriggerCandidateMaker::do_work(std::atomic<bool>& running_flag)
{
  triggeralgs::TimeStampedData data;

  while (running_flag.load()) {
    triggeralgs::TriggerCandidate candidate;

    try {
      inputQueue_->pop(data, queueTimeout_);
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      continue;
    }

    candidate = TimingTriggerCandidateMaker::TimeStampedDataToTriggerCandidate(data);

    TLOG_DEBUG(2) << "Activity received.";

    bool successfullyWasSent = false;
    while (!successfullyWasSent) {
      try {
        outputQueue_->push(candidate, queueTimeout_);
        successfullyWasSent = true;
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        std::ostringstream oss_warn;
        oss_warn << "push to output queue \"" << outputQueue_->get_name() << "\"";
        ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(),
          std::chrono::duration_cast<std::chrono::milliseconds>(queueTimeout_).count()));
      }
    }

    TLOG_DEBUG(2) << "Exiting do_work() method, received and successfully sent.";
  }
}

void
TimingTriggerCandidateMaker::do_scrap(const nlohmann::json&)
{}
} // namespace trigger
} // namespace dunedaq
