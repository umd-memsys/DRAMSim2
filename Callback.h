
#ifndef CALLBACK_H
#define CALLBACK_H

namespace DRAMSim {

template <typename ReturnT, typename Param1T, typename Param2T, 
                    typename Param3T>
class CallbackBase
{
    public:
        virtual ~CallbackBase() = 0;
        virtual ReturnT operator()(Param1T, Param2T, Param3T) = 0;
};

template <typename Return, typename Param1T, typename Param2T, typename Param3T>
DRAMSim::CallbackBase<Return,Param1T,Param2T,Param3T>::~CallbackBase(){}

template <typename ConsumerT, typename ReturnT, 
                        typename Param1T, typename Param2T, typename Param3T >
class Callback: public CallbackBase<ReturnT,Param1T,Param2T,Param3T>
{
    private:
        typedef ReturnT (ConsumerT::*PtrMember)(Param1T,Param2T,Param3T);

    public:
        Callback( ConsumerT* const object, PtrMember member) :
                object(object), member(member) {
        }

        Callback( const Callback<ConsumerT,ReturnT,Param1T,Param2T,Param3T>& e ) :
                object(e.object), member(e.member) {
        }

        ReturnT operator()(Param1T param1, Param2T param2, Param3T param3) {
            return (const_cast<ConsumerT*>(object)->*member)
                                                    (param1,param2,param3);
        }

    private:

        ConsumerT* const object;
        const PtrMember  member;
};

} // namespace DRAMSim 

#endif
