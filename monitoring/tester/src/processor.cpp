#include <boost/algorithm/string.hpp>

#include "config.h"
#include "processor.h"
#include "record.h"
#include "m_types.h"

namespace tester {
    /**
     * @brief Processor::Processor
     * @param seg_
     * @param container_
     */
    Processor::Processor(boost::interprocess::managed_mapped_file &seg)
        :seg_(seg),
          records_(nullptr),
          mutex_(bip::open_or_create, operation_mutex),
          s_(nullptr)
    {
        // create or open the record container in shared memory
        records_ = seg_.find_or_construct<record_container>("record container")(
                    record_container::ctor_args_list(),
                    record_container::allocator_type(seg.get_segment_manager()));

        // get statistics from storage
        s_ = seg_.find_or_construct<StatArray>("statistic container")(8,
                    record_container::allocator_type(seg.get_segment_manager()));
        //
        // GET routine
        //
        procMap_["get"] = [&](std::string _key, std::string value)->response {
            LOG_TRACE << "get start..";

            (void)value;

            if(!records_) {
                incFailed(GET);
                return {AppError,
                            errorBody(AppError)};
            }

            shared_string key(
                        shared_string::allocator_type(seg_.get_segment_manager()));
            key = _key.c_str();
            typedef nth_index<record_container, 0>::type index_by_name;
            index_by_name& idx = get<0>(*records_);
            index_by_name::iterator it;
            record r(shared_string::allocator_type(seg_.get_segment_manager()));
            {
                bip::scoped_lock<bip::named_mutex> lock(mutex_);
                it = idx.find(key);
                if(it == idx.end()){
                    incFailed(GET);
                    return {GetFailedByKeyNotPresent,
                                errorBody(GetFailedByKeyNotPresent)};
                }
                r = *it;
                std::string s(r.value.data(), r.value.size());
                incSuccess(GET);
                return {ExecSuccess, s};
            }

            incFailed(GET);
            return {GetFailedByKeyNotPresent,
                        errorBody(GetFailedByKeyNotPresent)};
        };

        //
        // INSERT routine
        //
        procMap_["insert"] = [&](std::string key, std::string value)->response {

            LOG_TRACE << "insert start..";

            if(!records_) {
                incFailed(INSERT);
                return {AppError,
                            errorBody(AppError)};
            }

            record r(shared_string::allocator_type(seg_.get_segment_manager()));
            r.key = key.c_str();
            r.value = value.c_str();

            bip::scoped_lock<bip::named_mutex> lock(mutex_);

            auto result = records_->insert(r);
            if(result.second) {
                incSuccess(INSERT);
                return {ExecSuccess,
                            errorBody(ExecSuccess)};
            }

            incFailed(INSERT);
            return {InsertFailedByKeyPresent,
                        errorBody(InsertFailedByKeyPresent)};
        };

        //
        // DELETE routines
        //
        procMap_["delete"] = [&](std::string _key, std::string value)->response {
            LOG_TRACE << "delete start..";

            (void)value;

            if(!records_) {
                incFailed(DELETE);
                return {AppError,
                            errorBody(AppError)};
            }

            shared_string key(
                        shared_string::allocator_type(seg_.get_segment_manager()));
            key = _key.c_str();
            typedef nth_index<record_container, 0>::type index_by_name;
            index_by_name& idx = get<0>(*records_);
            index_by_name::iterator it;
            record r(shared_string::allocator_type(seg_.get_segment_manager()));
            it = idx.find(key);
            if(it==idx.end()){
                incFailed(DELETE);
                return {DeleteFailedByKeyNotPresent,
                            errorBody(DeleteFailedByKeyNotPresent)};
            }
            r = *it;
            bip::scoped_lock<bip::named_mutex> lock(mutex_);
            idx.erase(it);

            incSuccess(DELETE);
            return {ExecSuccess, errorBody(ExecSuccess)};

        };

        //
        // UPDATE routines
        //
        procMap_["update"] = [&](std::string _key, std::string value)->response {
            LOG_TRACE << "update start..";

            if(!records_) {
                incFailed(UPDATE);
                return {AppError,
                            errorBody(AppError)};
            }

            shared_string key(
                        shared_string::allocator_type(seg_.get_segment_manager()));
            key = _key.c_str();
            typedef nth_index<record_container, 0>::type index_by_name;
            index_by_name& idx = get<0>(*records_);
            index_by_name::iterator it;
            record r(shared_string::allocator_type(seg_.get_segment_manager()));
            {
                bip::scoped_lock<bip::named_mutex> lock(mutex_);
                it = idx.find(key);
                if(it == idx.end()){
                    incFailed(UPDATE);
                    return {UpdateFailedByKeyNotPresent,
                                errorBody(UpdateFailedByKeyNotPresent)};
                }
                r = *it;
                if(r.value.compare(value.c_str()) == 0) {
                    incFailed(UPDATE);
                    return {UpdateFailedByKeyValueIsPresent,
                                errorBody(UpdateFailedByKeyValueIsPresent)};
                }
            }

            bip::scoped_lock<bip::named_mutex> lock(mutex_);
            idx.modify(it, [&](record &r){
                r.value = value.c_str();
            });

            incSuccess(UPDATE);
            return {ExecSuccess, errorBody(ExecSuccess)};
        };
    }


    /**
     * @brief Processor::exec
     * @param r
     * @return
     */
    response Processor::exec(request &r) {
        auto command = boost::algorithm::to_lower_copy(r.command);
        if(procMap_.find(command) != procMap_.end()) {
            return procMap_[command](r.key, r.value);
        }
        return {InvalidRequestCommand, "Inavlid request command.."};
    }

    /**
     * @brief Processor::incSuccess
     * @param op
     */
    void Processor::incSuccess(Operation op) {
        if(s_ == nullptr) {
            return;
        }
        (*s_)[(int)Success + (int)op] += 1;
    }

    /**
     * @brief Processor::incFailed
     * @param op
     */
    void Processor::incFailed(Operation op) {
        if(s_ == nullptr) {
            return;
        }
        (*s_)[(int)Failed + (int)op] += 1;
    }

    /**
     * @brief Processor::echoStatus
     * @return
     */
    std::ostringstream Processor::echoStatus() {
        if(s_ == nullptr || records_ == nullptr) {
            return {};
        }
        std::ostringstream outStream;
        outStream << "RECORDS:" << records_->size() << ".";
        outStream << "INSERT:[S]=" << (*s_)[0] << "/[F]=" << (*s_)[4] << ".";
        outStream << "UPDATE:[S]=" << (*s_)[1] << "/[F]=" << (*s_)[5] << ".";
        outStream << "DELETE:[S]=" << (*s_)[2] << "/[F]=" << (*s_)[6] << ".";
        outStream << "GET:[S]=" << (*s_)[3] << "/[F]=" << (*s_)[7];
        return outStream;
    }


}
