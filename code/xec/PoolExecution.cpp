#include "PoolExecution.hpp"
#include "ExecutorBase.hpp"

#include <itlib/qalgorithm.hpp>

namespace xec {

void PoolExecution::thread() {
    while (true) {
        {
            std::lock_guard l(m_strandMutex);

            // clear strands which are not running
            itlib::erase_all_if(m_strands, [](const Strand& s) {
                if (s.context->running()) return false;
                s.executor.finalize(); // use this loop to finalize
                return true;
            });

            // only stop if all strands are stopped
            if (m_strands.empty() && m_stopping) return;

            for (auto& s : m_strands) {

            }
        }
    }
}

}
