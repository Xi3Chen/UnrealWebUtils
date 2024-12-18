// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Interfaces/IHttpRequest.h"
#include "HTTPRequest.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FSimpleHttpRequestCompleteAsStringDelegate, bool, bSuccess, FString, ContentString);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FSimpleHttpRequestCompleteAsBinaryDelegate, bool, bSuccess, const TArray<uint8>&, ContentBinary);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FSimpleHttpRequestHeaderReceivedDelegate,const FString&, HeaderName, const FString&, NewHeaderValue);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FSimpleHttpRequestProgressDelegate, int32, BytesReceived, int32, ContentLength);
DECLARE_DYNAMIC_DELEGATE_OneParam(FSimpleHttpRequestWillRetryDelegate,float ,SecondsToRetry);

/**
 * 
 */
UCLASS(BlueprintType)
class SIMPLEHTTPMODULE_API UHTTPRequest : public UObject
{
	GENERATED_BODY()

public:
	FSimpleHttpRequestCompleteAsStringDelegate OnRequestCompleteAsString;
	FSimpleHttpRequestCompleteAsBinaryDelegate OnRequestCompleteAsBinary;
	FSimpleHttpRequestHeaderReceivedDelegate OnRequestHeaderReceived;
	FSimpleHttpRequestProgressDelegate OnRequestProgress;
	FSimpleHttpRequestWillRetryDelegate OnRequestWillRetry;

	UFUNCTION(BlueprintCallable, Category = "SimpleHTTPModule|Request", meta = (DisplayName = "绑定请求完成的委托（字符类型）"))
	void BindRequestCompleteAsString(FSimpleHttpRequestCompleteAsStringDelegate InDelegate);

	UFUNCTION(BlueprintCallable, Category = "SimpleHTTPModule|Request", meta = (DisplayName = "绑定请求完成的委托（二进制类型）"))
	void BindRequestCompleteAsBinary(FSimpleHttpRequestCompleteAsBinaryDelegate InDelegate);

	UFUNCTION(BlueprintCallable, Category = "SimpleHTTPModule|Request", meta = (DisplayName = "绑定请求头部接收的委托"))
	void BindRequestHeaderReceived(FSimpleHttpRequestHeaderReceivedDelegate InDelegate);

	UFUNCTION(BlueprintCallable, Category = "SimpleHTTPModule|Request",meta = (DisplayName = "绑定接受数据进程的委托"))
	void BindRequestProgress(FSimpleHttpRequestProgressDelegate InDelegate);

	//bind event on Request WillRetry
	UFUNCTION(BlueprintCallable, Category = "SimpleHTTPModule|Request", meta = (DisplayName = "绑定请求将重试的委托"))
	void BindRequestWillRetry(FSimpleHttpRequestWillRetryDelegate InDelegate);

	/**
	* 保存接受的文件
	* @param SavePath 报错文件路径不能为空。
	* @param FileName 文件名。可以为空，但是UsingReceivedFileName必须为true。
	* @param UsingReceivedFileName 使用默认服务器给定的文件名。当为false时则使用FileName变量作为文件名。
	*/
	UFUNCTION(BlueprintCallable, Category = "SimpleHTTPModule|Request", meta = (DisplayName = "保存接收的文件"))
	bool SaveAsFile(FString SavePath,FString FileName,bool UsingReceivedFileName = true);

	UFUNCTION(BlueprintCallable, Category = "SimpleHTTPModule|Request", meta = (DisplayName = "释放请求内存空间"))
	void FreeRequest();

	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = nullptr;
	//TArray<uint8> BinaryContent;
private:
	UPROPERTY()
	class UHTTPHelperSubsystem* HTTPHelperSubsystem = nullptr;
	friend class UHTTPHelperSubsystem;

	void BindAllDelegate(const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> InHttpRequest);

	void OnProcessRequestCompleteEvent(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnHeaderReceivedEvent(FHttpRequestPtr Request, const FString& HeaderName, const FString& NewHeaderValue);
	void OnRequestProgressEvent(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived);
	void OnRequestWillRetryEvent(FHttpRequestPtr Request, FHttpResponsePtr Response, float AttemptNumber);
};
