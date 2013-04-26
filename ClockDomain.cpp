#include "ClockDomain.h"

using namespace std;



namespace ClockDomain
{
	// "Default" crosser with a 1:1 ratio
	ClockDomainCrosser::ClockDomainCrosser(ClockUpdateCB *_callback)
		: callback(_callback), clock1(1UL), clock2(1UL), counter1(0UL), counter2(0UL)
	{
	}
	ClockDomainCrosser::ClockDomainCrosser(uint64_t _clock1, uint64_t _clock2, ClockUpdateCB *_callback) 
		: callback(_callback), clock1(_clock1), clock2(_clock2), counter1(0), counter2(0)
	{
		//cout << "CTOR: callback address: " << (uint64_t)(this->callback) << "\t ratio="<<clock1<<"/"<<clock2<< endl;
	}

	void ClockDomainCrosser::update()
	{
		//short circuit case for 1:1 ratios
		if (clock1 == clock2 && callback)
		{
			(*callback)();
			return; 
		}

		// Update counter 1.
		counter1 += clock1;

		while (counter2 < counter1)
		{
			counter2 += clock2;
			//cout << "CALLBACK: counter1= " << counter1 << "; counter2= " << counter2 << "; " << endl;
			//cout << "callback address: " << (uint64_t)callback << endl;
			if (callback)
			{
				//cout << "Callback() " << (uint64_t)callback<< "Counters: 1="<<counter1<<", 2="<<counter2 <<endl;
				(*callback)();
			}
		}

		if (counter1 == counter2)
		{
			counter1 = 0;
			counter2 = 0;
		}
	}



	void TestObj::cb()
	{
			cout << "In Callback\n";
	}

	int TestObj::test()
	{
		ClockUpdateCB *callback = new Callback<TestObj, void>(this, &TestObj::cb);

		ClockDomainCrosser x(5,2,callback);
		//ClockDomainCrosser x(2,5,NULL);
		//ClockDomainCrosser x(37,41,NULL);
		//ClockDomainCrosser x(41,37,NULL);
		//cout << "(main) callback address: " << (uint64_t)&cb << endl;


		for (int i=0; i<10; i++)
		{
			
			x.update();
			cout << "UPDATE: counter1= " << x.counter1 << "; counter2= " << x.counter2 << "; " << endl;
		}

		return 0;
	}


}
