// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"


void UMyBlueprintFunctionLibrary::AsyncPrimeCount(FTestDelegate Out, FTestDoneDelegate Done, int32 PrimeCount)
{
	// Schedule a thread                               // Pass in our parameters to the lambda expression
	AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [Out, Done, PrimeCount]()
	{
		// This whole code just loops through numbers and counts how many prime nums it finds
		int PrimesFound = 0;
		int TestNumber = 2;

		while (PrimesFound < PrimeCount)
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
					AsyncTask(ENamedThreads::GameThread, [Out, PrimesFound]()
					{
						// We execute the delegate along with the param
						Out.ExecuteIfBound(FString::FromInt(PrimesFound));
					});
				}
			}
			TestNumber++;
		}

		// back to main thread
		AsyncTask(ENamedThreads::GameThread, [Done]()
		{
			Done.ExecuteIfBound();
		});
	});
}
