// Fill out your copyright notice in the Description page of Project Settings.


#include "HTTPRequest.h"
#include "Interfaces/IHttpResponse.h"


void UHTTPRequest::BindRequestCompleteAsString(FSimpleHttpRequestCompleteAsStringDelegate InDelegate)
{
	OnRequestCompleteAsString = InDelegate;
}

void UHTTPRequest::BindRequestCompleteAsBinary(FSimpleHttpRequestCompleteAsBinaryDelegate InDelegate)
{
	OnRequestCompleteAsBinary = InDelegate;
}

void UHTTPRequest::BindRequestHeaderReceived(FSimpleHttpRequestHeaderReceivedDelegate InDelegate)
{
	OnRequestHeaderReceived = InDelegate;
}

void UHTTPRequest::BindRequestProgress(FSimpleHttpRequestProgressDelegate InDelegate)
{
	OnRequestProgress = InDelegate;
}

void UHTTPRequest::BindRequestWillRetry(FSimpleHttpRequestWillRetryDelegate InDelegate)
{
	OnRequestWillRetry = InDelegate;
}

bool UHTTPRequest::SaveAsFile(FString SavePath, FString FileName, bool UsingReceivedFileName)
{
	if (HttpRequest.IsValid() && HttpRequest->GetResponse().IsValid() && EHttpResponseCodes::IsOk(HttpRequest->GetResponse()->GetResponseCode()))
	{
		//save uint8 array to file
		if (UsingReceivedFileName)
		{
			TArray<FString> AllHeaders = HttpRequest->GetResponse()->GetAllHeaders();
			for (auto Header : AllHeaders)
			{
				if (Header.StartsWith("Content-Disposition"))
				{
					int32 FileNameStartIndex = Header.Find("filename=\"");
					if (FileNameStartIndex != INDEX_NONE)
					{
						FileName = Header.Mid(FileNameStartIndex + 10).Replace(TEXT("\""), TEXT(""));
						UE_LOG(LogTemp, Log, TEXT("File name: %s will be saved"), *FileName);
					}
					break;
				}
			}
			FileName = FPaths::GetCleanFilename(HttpRequest->GetURL());
		}
		SavePath = FPaths::Combine(SavePath, FileName);
		return FFileHelper::SaveArrayToFile(BinaryContent, *SavePath);
	}
	return false;
}

void UHTTPRequest::BindAllDelegate(const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> InHttpRequest)
{
	BinaryContent.Empty();
	HttpRequest = InHttpRequest;
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UHTTPRequest::OnProcessRequestCompleteEvent);

	HttpRequest->OnHeaderReceived().BindUObject(this, &UHTTPRequest::OnHeaderReceivedEvent);

	HttpRequest->OnRequestProgress().BindUObject(this, &UHTTPRequest::OnRequestProgressEvent);

	HttpRequest->OnRequestWillRetry().BindUObject(this, &UHTTPRequest::OnRequestWillRetryEvent);
}

void UHTTPRequest::OnProcessRequestCompleteEvent(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	BinaryContent = Response->GetContent();
	OnRequestCompleteAsString.ExecuteIfBound(bWasSuccessful, Response->GetContentAsString());
	OnRequestCompleteAsBinary.ExecuteIfBound(bWasSuccessful, Response->GetContent());
}

void UHTTPRequest::OnHeaderReceivedEvent(FHttpRequestPtr Request, const FString& HeaderName, const FString& NewHeaderValue)
{
	OnRequestHeaderReceived.ExecuteIfBound(HeaderName, NewHeaderValue);
}

void UHTTPRequest::OnRequestProgressEvent(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
{
	OnRequestProgress.ExecuteIfBound(BytesReceived, Request->GetResponse()->GetContentLength());
}

void UHTTPRequest::OnRequestWillRetryEvent(FHttpRequestPtr Request, FHttpResponsePtr Response, float AttemptNumber)
{
	OnRequestWillRetry.ExecuteIfBound(AttemptNumber);
}
