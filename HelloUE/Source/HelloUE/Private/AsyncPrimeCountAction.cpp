// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncPrimeCountAction.h"


UAsyncPrimeCountAction* UAsyncPrimeCountAction::AsyncPrimeCount(int PrimeCount)
{
	UAsyncPrimeCountAction* BPNode = NewObject<UAsyncPrimeCountAction>();
	BPNode->PrimeCount = PrimeCount;
	return BPNode;
}

void UAsyncPrimeCountAction::Activate()
{
	// Schedule a thread                               // Pass in our parameters to the lambda expression
	AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [this]()
	{
		// This whole code just loops through numbers and counts how many prime nums it finds
		int PrimesFound = 0;
		int TestNumber = 2;

		while (PrimesFound < this->PrimeCount)
		{
			bool bIsPrime = true;

			for (int i = 2; i < TestNumber / 2; i++)
			{
				if (TestNumber % i == 0)
				{
					bIsPrime = false;
					break;
				}
			}

			if (bIsPrime)
			{
				PrimesFound++;

				// Every 1000 prime...
				if (PrimesFound % 1000 == 0)
				{
					// We schedule back to the main thread and pass in our params
					AsyncTask(ENamedThreads::GameThread, [this, PrimesFound]()
					{
						// We execute the delegate along with the param
						this->OnProgress.Broadcast(FString::FromInt(PrimesFound));
					});
				}
			}
			TestNumber++;
		}

		// back to main thread
		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			this->OnFinish.Broadcast();
		});
	});
}
