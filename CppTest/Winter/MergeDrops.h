// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MergeDrops.generated.h"


/**
 * 
 */
UCLASS()
class CPPTEST_API UMergeDrops : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "MyFunction|MyClass")
        static int MyFunc(int val);
    
};
