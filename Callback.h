/****************************************************************************
*	 DRAMSim2: A Cycle Accurate DRAM simulator 
*	 
*	 Copyright (C) 2010   	Elliott Cooper-Balis
*									Paul Rosenfeld 
*									Bruce Jacob
*									University of Maryland
*
*	 This program is free software: you can redistribute it and/or modify
*	 it under the terms of the GNU General Public License as published by
*	 the Free Software Foundation, either version 3 of the License, or
*	 (at your option) any later version.
*
*	 This program is distributed in the hope that it will be useful,
*	 but WITHOUT ANY WARRANTY; without even the implied warranty of
*	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	 GNU General Public License for more details.
*
*	 You should have received a copy of the GNU General Public License
*	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************/
#ifndef CALLBACK_H
#define CALLBACK_H

namespace DRAMSim
{

template <typename ReturnT, typename Param1T, typename Param2T,
typename Param3T>
class CallbackBase
{
public:
	virtual ~CallbackBase() = 0;
	virtual ReturnT operator()(Param1T, Param2T, Param3T) = 0;
};

template <typename Return, typename Param1T, typename Param2T, typename Param3T>
DRAMSim::CallbackBase<Return,Param1T,Param2T,Param3T>::~CallbackBase() {}

template <typename ConsumerT, typename ReturnT,
typename Param1T, typename Param2T, typename Param3T >
class Callback: public CallbackBase<ReturnT,Param1T,Param2T,Param3T>
{
private:
	typedef ReturnT (ConsumerT::*PtrMember)(Param1T,Param2T,Param3T);

public:
	Callback( ConsumerT* const object, PtrMember member) :
			object(object), member(member)
	{
	}

	Callback( const Callback<ConsumerT,ReturnT,Param1T,Param2T,Param3T>& e ) :
			object(e.object), member(e.member)
	{
	}

	ReturnT operator()(Param1T param1, Param2T param2, Param3T param3)
	{
		return (const_cast<ConsumerT*>(object)->*member)
		       (param1,param2,param3);
	}

private:

	ConsumerT* const object;
	const PtrMember  member;
};

typedef CallbackBase <void, uint, uint64_t, uint64_t> TransactionCompleteCB;
} // namespace DRAMSim

#endif
