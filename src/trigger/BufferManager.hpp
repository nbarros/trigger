/**
 * @file BufferManager.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2021.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TRIGGER_SRC_TRIGGER_BUFFERMANAGER_HPP_
#define TRIGGER_SRC_TRIGGER_BUFFERMANAGER_HPP_

#include "dataformats/Types.hpp"
#include "trigger/TPSet.hpp"

#include <set>

namespace dunedaq {
namespace trigger {

/**
 * @brief BufferManager description.
 *
 */
class BufferManager
{
public:
  BufferManager();

  virtual ~BufferManager();

  BufferManager(BufferManager const&) = delete;
  BufferManager(BufferManager&&) = default;
  BufferManager& operator=(BufferManager const&) = delete;
  BufferManager& operator=(BufferManager&&) = default;

  /**
   *  add a TPSet to the buffer. Remove oldest TPSets from buffer if we are at maximum size
   */
  bool add(trigger::TPSet& tps);
  
  /**
   * return a vector of all the TPSets in the buffer that overlap with [start_time, end_time]
   */
  std::vector<trigger::TPSet> get_tpsets_in_window(dataformats::timestamp_t start_time, dataformats::timestamp_t end_time);

private:

  //Compare start_time of a TPSet when adding to the buffer
  struct TPSetCmp {
    bool operator()(const TPSet& ltps, const TPSet& rtps) const {
      dataformats::timestamp_t const LTPS = ltps.start_time;
      dataformats::timestamp_t const RTPS = rtps.start_time;
      return LTPS < RTPS;
    }
  };

  //Where the TPSet will be buffered
  std::set<trigger::TPSet,TPSetCmp> m_tpset_buffer;

};

} // namespace trigger
} // namespace dunedaq

#endif // TRIGGER_SRC_TRIGGER_BUFFERMANAGER_HPP_
