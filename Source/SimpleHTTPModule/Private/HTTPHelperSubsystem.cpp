// Fill out your copyright notice in the Description page of Project Settings.


#include "HTTPHelperSubsystem.h"
#include "HttpModule.h"
#include "GenericPlatform/GenericPlatformHttp.h"


FHttpRequestFileWapper::FHttpRequestFileWapper(const FString& InKeyName, const FString& InFilePah)
{
	KeyName = InKeyName;
	FilePath = InFilePah;
	LoadFile();
}

FHttpRequestFileWapper::FHttpRequestFileWapper(const FString& InKeyName, const FString& InFileName, const TArray64<uint8>& InFileContent)
{
	KeyName = InKeyName;
	FileName = InFileName;
	bIsFile = true;
	FilePath = InFileName;
	if (InFileContent.Num() > 0)
	{
		FileContent = InFileContent;
	}
}

FHttpRequestFileWapper::FHttpRequestFileWapper(const FString& InKeyName, const FString& InFileName, TArray64<uint8>&& InFileContent)
{
	KeyName = InKeyName;
	FileName = InFileName;
	bIsFile = true;
	FilePath = InFileName;
	if (InFileContent.Num() > 0)
	{
		FileContent = InFileContent;
	}
}

bool FHttpRequestFileWapper::CheckValid() const
{
	if(FilePath.IsEmpty() && bIsFile)
	{
		return false;
	}
	if(FileContent.Num() == 0)
	{
		return false;
	}
	return true;
}

bool FHttpRequestFileWapper::LoadFile()
{
	if (FilePath.Len()>2)
	{
		if (FFileHelper::LoadFileToArray(FileContent, *FilePath))
		{
			bIsFile = true;
			FileName = FPaths::GetCleanFilename(FilePath);
			return true;
		}
	}
	bIsFile = false;
	
	return false;
}

UHTTPRequest* UHTTPHelperSubsystem::CallHTTP(FString URL, EMethodByte Verb, TMap<FString, FString> Headers, TMap<FString, FString> Params, FString Content, float InTimeoutSecs, bool bAddDefaultHeaders)
{
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHTTP_Native(URL,Verb,Headers,Params,InTimeoutSecs,bAddDefaultHeaders);
	HttpRequest->SetContentAsString(Content);
	return CreateHttpRequestObject(HttpRequest);
}

UHTTPRequest* UHTTPHelperSubsystem::CallHTTPAndLoadFile(FString URL, EMethodByte Verb, TMap<FString, FString> Headers, TMap<FString, FString> Params, FString FilePath, float InTimeoutSecs,bool bAddDefaultHeaders)
{
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHTTP_Native(URL, Verb, Headers, Params, InTimeoutSecs, bAddDefaultHeaders);
	HttpRequest->SetContentAsStreamedFile(FilePath);
	return CreateHttpRequestObject(HttpRequest,-1);
}

UHTTPRequest* UHTTPHelperSubsystem::CallHTTPAsBinary(FString URL, EMethodByte Verb, TMap<FString, FString> Headers, TMap<FString, FString> Params, TArray<uint8> Content, int32 ContentLength, float InTimeoutSecs,bool bAddDefaultHeaders)
{
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = CreateHTTP_Native(URL, Verb, Headers, Params, InTimeoutSecs, bAddDefaultHeaders);
	HttpRequest->SetContent(Content);
	return CreateHttpRequestObject(HttpRequest,ContentLength);
}

UHTTPRequest* UHTTPHelperSubsystem::CallHTTPAsFiles(FString URL, TMap<FString, FString> Headers, TMap<FString, FString> Params, TArray<FHttpRequestFileCreator>& Files, EMethodByte Verb,float InTimeoutSecs, bool bAddDefaultHeaders)
{
	if (Files.Num() == 0)
	{
		return nullptr;
	}
	TArray< FHttpRequestFileWapperBase*> FileWapper;
	for (int i=0;i<Files.Num();i++)
	{
		if (Files[i].GenerateFileWapper().IsValid())
		{
			FileWapper.Add(Files[i].GetCreateWapper().Get());
		}
	}
	TMap<FString, FString> LocalHeader;
	TArray<uint8> Content;
	int32 ContentLength = 0;
	if (CreateFileContentAndHeadersForFromData(FileWapper, LocalHeader, Content, ContentLength))
	{
		for (auto it : Headers)
		{
			if (FString* FindedValue = LocalHeader.Find(it.Key))
			{
				continue;
			}
			else
			{
				LocalHeader.Add(it.Key, it.Value);
			}
		}
		return CallHTTPAsBinary(URL, Verb, LocalHeader, Params, Content, ContentLength, InTimeoutSecs, bAddDefaultHeaders);
	}
	return nullptr;
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UHTTPHelperSubsystem::CreateHTTP_Native(FString URL, const EMethodByte& Verb, const TMap<FString, FString>& Headers, const TMap<FString, FString>& Params, float InTimeoutSecs, bool bAddDefaultHeaders)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	int ParamsCount = 0;
	for (auto Param : Params)
	{
		if (ParamsCount == 0)
		{
			URL = URL + "?";
		}
		else
		{
			URL = URL + "&";
		}
		URL = URL + Param.Key + "=" + Param.Value;
		ParamsCount++;
	}
	HttpRequest->SetURL(URL);
	HttpRequest->SetVerb(MethodByteToString(Verb));
	for (auto Header : Headers)
	{
		HttpRequest->AppendToHeader(Header.Key, Header.Value);
	}
	if (bAddDefaultHeaders)
	{
		for(auto Header : DefaultHeaders)
		{
			if (!Headers.Contains(Header.Key))
			{
				HttpRequest->AppendToHeader(Header.Key, Header.Value);
			}
		}

	}
	HttpRequest->SetTimeout(InTimeoutSecs);
	return HttpRequest;
}

UHTTPRequest* UHTTPHelperSubsystem::CreateHttpRequestObject(const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& HttpRequest, uint32 ContentLength)
{
	if (ContentLength == 0)
	{
		FString Content = FString::FromInt(HttpRequest->GetContent().Num());
		// cull , char in Content
		Content.ReplaceInline(TEXT(","), TEXT(""), ESearchCase::CaseSensitive);
		HttpRequest->SetHeader("Content-Length", Content);
	}
	if (HttpRequest->ProcessRequest())
	{
		UHTTPRequest* HttpRequestObject = NewObject<UHTTPRequest>();
		HttpRequestObject->BindAllDelegate(HttpRequest);
		HistoryHttpRequests.Add(HttpRequestObject);
		return HttpRequestObject;
	}
	return nullptr;
}

bool UHTTPHelperSubsystem::CreateFileContentAndHeadersForFromData(const TArray<FHttpRequestFileWapperBase*>& FileWappers,
	TMap<FString, FString>& Headers, TArray<uint8>& Content,
	int32& ContentLength)
{
	if(FileWappers.Num() == 0)
	{
		return false; 
	}
	FString Boundary = "----" + FString(TEXT("WebKitFormBoundary"))+ FString::FromInt(FMath::Abs( FDateTime::Now().GetMillisecond()))+ FGuid::NewGuid().ToString();
	Headers.Add("Content-Type", TEXT("multipart/form-data; boundary=" + Boundary));
	FString BoundaryLine = "\r\n--" + Boundary + "\r\n";
	for (const auto& FileWapper : FileWappers)
	{
		if (!FileWapper->CheckValid())
		{
			return false;
		}
		Content.Append((uint8*)TCHAR_TO_ANSI(*BoundaryLine), BoundaryLine.Len());
		ContentLength += FileWapper->FileContent.Num();
		FString FileHeader = "Content-Disposition: form-data;name=\"" + FileWapper->KeyName + "\"";
		//check if it is a file not string 
		if(FileWapper->bIsFile)
		{
			const FHttpRequestFileWapper* LocalFileWapper = (FHttpRequestFileWapper *)(FileWapper);
			FileHeader.Append(";");
			FileHeader.Append("filename=\"" + FPaths::GetCleanFilename(LocalFileWapper->FilePath) + "\"\r\n");
			FileHeader.Append("Content-Type: "+ GetFileContentType(LocalFileWapper->FilePath) + "\r\n\r\n");
		}
		const FTCHARToUTF8  FileHeaderChar = FTCHARToUTF8(*FileHeader);
		Content.Append((uint8*)FileHeaderChar.Get(), FileHeaderChar.Length());
		Content.Append(FileWapper->FileContent);
		FString FileEndLine = TEXT("\r\n");
		Content.Append((uint8*)TCHAR_TO_ANSI(*FileEndLine), FileEndLine.Len());
	}
	{
		FString EndBoundary = "\r\n--" + Boundary + FString("--") + "\r\n";
		Content.Append((uint8*)TCHAR_TO_ANSI(*(EndBoundary)), EndBoundary.Len());
		ContentLength = Content.Num();
	}
	FString ContentLengthStr = FString::FromInt(ContentLength);
	ContentLengthStr.ReplaceInline(TEXT(","), TEXT(""), ESearchCase::CaseSensitive);

	Headers.Add("Content-Length", ContentLengthStr);
	return true;
}

FString UHTTPHelperSubsystem::MethodByteToString(EMethodByte MethodByte)
{
	switch (MethodByte)
	{
	case EMethodByte::GET:
		return "GET";
		break;
	case EMethodByte::POST:
		return "POST";
		break;
	case EMethodByte::PUT:
		return "PUT";
		break;
	case EMethodByte::PATCH:
		return "PATCH";
		break;
	case EMethodByte::DEL:
		return "DELETE";
		break;
	case EMethodByte::COPY:
		return "COPY";
		break;
	case EMethodByte::HEAD:
		return "HEAD";
		break;
	case EMethodByte::OPTIONS:
		return "OPTIONS";
		break;
	case EMethodByte::LINK:
		return "LINK";
		break;
	case EMethodByte::UNLINK:
		return "UNLINK";
		break;
	case EMethodByte::LOCK:
		return "LOCK";
		break;
	case EMethodByte::UNLOCK:
		return "UNLOCK";
		break;
	case EMethodByte::PROPFIND:
		return "PROPFIND";
		break;
	case EMethodByte::VIEW:
		return "VIEW";
		break;
	default:
		return "GET";
		break;
	}
}

FString UHTTPHelperSubsystem::ConvertPathToLinuxPath(FString Path)
{
	FString LinuxPath;
	for (int i = 0; i < Path.Len(); i++)
	{
		if(i == 0 && Path[i] != '/')
		{
			LinuxPath += '/';
		}
		if (Path[i] == '\\')
		{
			LinuxPath += '/';
		}
		else
		{
			LinuxPath += Path[i];
		}
	}
	return LinuxPath;
}

FString UHTTPHelperSubsystem::GetFileContentType(const FString& FilePath)
{
	return FGenericPlatformHttp::GetMimeType(FilePath);
}

FHttpRequestFileWapperBase::FHttpRequestFileWapperBase(const FString& InKeyName, const FString& TextContent)
{
	KeyName = InKeyName;
	const FTCHARToUTF8 LocalString = FTCHARToUTF8(*TextContent);
	FileContent.Empty();
	FileContent.Append((uint8*)LocalString.Get(), LocalString.Length());
}

bool FHttpRequestFileWapperBase::CheckValid() const
{
	if (FileContent.Num() == 0 || KeyName.IsEmpty())
	{
		return false;
	}

	return true;
}

TSharedPtr<FHttpRequestFileWapperBase> FHttpRequestFileCreator::GenerateFileWapper()
{
	if (KeyName.Len() == 0)
	{
		return nullptr;

	}
	if (ContentInfo.Len() == 0 && bIsFile)
	{
		return nullptr;

	}
	if (bIsFile)
	{
		if (UploadContent.Num() > 0)
		{
			CreateWapper = MakeShareable(new FHttpRequestFileWapper(KeyName, ContentInfo, TArray64<uint8>(UploadContent)));
			if (!CreateWapper->CheckValid())
			{
				CreateWapper.Reset();
				return nullptr;
			}
		}
		else
		{
			
			CreateWapper = MakeShareable(new FHttpRequestFileWapper(KeyName, ContentInfo));
			if (!CreateWapper->CheckValid())
			{
				CreateWapper.Reset();
				return nullptr;
			}
		}
	}
	else
	{
		CreateWapper = MakeShareable(new FHttpRequestFileWapperBase(KeyName, ContentInfo));
		if (!CreateWapper->CheckValid())
		{
			CreateWapper.Reset();
			return nullptr;
		}
	}
	return CreateWapper;
}

FHttpRequestFileCreator::~FHttpRequestFileCreator()
{
	if (CreateWapper.IsValid())
	{
		CreateWapper.Reset();
	}
}
