// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HTTPRequest.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "HTTPRequest.h"
#include "HTTPHelperSubsystem.generated.h"

UENUM(BlueprintType)
enum class EHttpHelperContentType :uint8
{
	Text_Xml UMETA(DisplayName="text_html"),
	text_html UMETA(DisplayName = "text_html"),
	text_plain UMETA(DisplayName = "text_plain"),
	image_gif UMETA(DisplayName="image_gif"),
	image_jpeg UMETA(DisplayName = "image_jpeg"),
	image_png UMETA(DisplayName = "image_png"),
	application_xml UMETA(DisplayName = "application_xml"),
	application_json UMETA(DisplayName = "application_json"),
	application_octet_stream UMETA(DisplayName="application_octet_stream"),
	application_x_www_form_urlencoded UMETA(DisplayName = "application_x_www_form_urlencoded"),

};

UENUM(BlueprintType)
enum class EMethodByte : uint8
{
	GET,
	POST,
	PUT,
	PATCH,
	DEL UMETA(DisplayName = "DELETE"),
	COPY,
	HEAD,
	OPTIONS,
	LINK,
	UNLINK,
	LOCK,
	UNLOCK,
	PROPFIND,
	VIEW
	
};

USTRUCT(BlueprintType)
struct SIMPLEHTTPMODULE_API FHttpRequestFileCreator
{
	GENERATED_BODY()
public:
	//如果bIsFile 为true，则ContentInfo作为文件路径加载内容，或者使用UploadContent上传（但任然需要将文件名给到ContentInfo中，注意！使用该方式则有内存大小限制。请优先使用ContentInfo作为文件路径）。否则将ContentInfo作为上传的文本。
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SimpleHTTP")
	bool bIsFile = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SimpleHTTP")
	FString KeyName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SimpleHTTP")
	FString ContentInfo;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SimpleHTTP")
	TArray<uint8> UploadContent;

	TSharedPtr<FHttpRequestFileWapperBase> GenerateFileWapper();

	~FHttpRequestFileCreator();
	TSharedPtr<FHttpRequestFileWapperBase> GetCreateWapper() const
	{
		return CreateWapper;
	}
private:
	TSharedPtr<FHttpRequestFileWapperBase> CreateWapper = nullptr;

};

USTRUCT(BlueprintType)
struct SIMPLEHTTPMODULE_API FHttpRequestFileWapperBase
{
	GENERATED_BODY()
public:
	FHttpRequestFileWapperBase() = default;
	FHttpRequestFileWapperBase(const FString& InKeyName,const FString& TextContent);
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SimpleHTTP")
	bool bIsFile = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SimpleHTTP")
	FString KeyName;
	TArray64<uint8> FileContent;
	virtual bool CheckValid() const;

};

USTRUCT(BlueprintType)
struct FHttpRequestFileWapper:public FHttpRequestFileWapperBase
{
	GENERATED_BODY()
public:
	FHttpRequestFileWapper() = default;
	FHttpRequestFileWapper(const FString& InKeyName,const FString& InFilePah);

	FHttpRequestFileWapper(const FString& InKeyName, const FString& InFileName, const TArray64<uint8>& InFileContent);
	FHttpRequestFileWapper(const FString& InKeyName,const FString& InFileName,  TArray64<uint8>&& InFileContent);
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category = "SimpleHTTP")
	FString FilePath;
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category = "SimpleHTTP")
	FString FileName;

	virtual bool CheckValid() const override;

	bool LoadFile();
};

/**
 * 
 */
UCLASS()
class SIMPLEHTTPMODULE_API UHTTPHelperSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable,Category = "SimpleHTTP",DisplayName = "开始HTTP请求")
	UHTTPRequest* CallHTTP(
		FString URL,
		EMethodByte Verb,
		TMap<FString, FString> Headers,
		TMap<FString, FString> Params,
		FString Content,
		float InTimeoutSecs = 100,
		bool bAddDefaultHeaders = true);
	UFUNCTION(BlueprintCallable,Category = "SimpleHTTP",DisplayName = "上传文件的HTTP请求")
	UHTTPRequest* CallHTTPAndUploadFile(
		FString URL,
		EMethodByte Verb,
		TMap<FString, FString> Headers,
		TMap<FString, FString> Params,
		FString FilePath,
		float InTimeoutSecs = 100,
		bool bAddDefaultHeaders = true);

	UFUNCTION(BlueprintCallable,Category = "SimpleHTTP",DisplayName = "二进制的Content提交HTTP请求")
	UHTTPRequest* CallHTTPAsBinary(
		FString URL,
		EMethodByte Verb,
		TMap<FString, FString> Headers,
		TMap<FString, FString> Params,
		TArray<uint8> Content,
		int32 ContentLength = 0,
		float InTimeoutSecs = 100,
		bool bAddDefaultHeaders = true);

	UFUNCTION(BlueprintCallable, Category = "SimpleHTTP", DisplayName = "多文件的Content提交HTTP请求")
	UHTTPRequest* CallHTTPAsFiles(
		FString URL,
		TMap<FString, FString> Headers,
		TMap<FString, FString> Params,
		UPARAM(ref) TArray<FHttpRequestFileCreator>& Files,
		EMethodByte Verb = EMethodByte::POST,
		float InTimeoutSecs = 100,
		bool bAddDefaultHeaders = true);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CreateHTTP_Native(
		FString URL,
		const EMethodByte& Verb,
		const TMap<FString, FString>& Headers,
		const TMap<FString, FString>& Params,
		float InTimeoutSecs = 100,
		bool bAddDefaultHeaders = true);

	UHTTPRequest* CreateHttpRequestObject(const TSharedRef<IHttpRequest, ESPMode::ThreadSafe>& HttpRequest,uint32 ContentLength = 0);
	//创建多个上传文件内容的Content
	bool CreateFileContentAndHeadersForFromData(
	const TArray<FHttpRequestFileWapperBase*>& FileWappers,
		TMap<FString, FString> & Headers,
		TArray<uint8>& Content,
		int32& ContentLength
		);

	UFUNCTION(BlueprintPure, Category = "SimpleHTTP")
	static FString MethodByteToString(EMethodByte MethodByte);
	
	UFUNCTION(BlueprintPure, Category = "SimpleHTTP")
	static TMap<FString, FString> MakeDefaultContentType(EHttpHelperContentType HttpContentType = EHttpHelperContentType::application_json);

	UPROPERTY(BlueprintReadOnly,VisibleAnywhere, Category = "SimpleHTTP")
	TSet<UHTTPRequest*> HistoryHttpRequests;

	

	static FString ConvertPathToLinuxPath(FString Path);
	//计算文件对应的ContentType
	static FString GetFileContentType(const FString& FilePath);
	UPROPERTY(BlueprintReadWrite, Category = "SimpleHTTP")
	TMap<FString, FString> DefaultHeaders = { {"Content-Type","application/x-www-form-urlencoded"},
		{"User-Agent","UnrealWebUtilsByXiChen"} ,
		{"Accept", "*/*" },
	{ "Accept-Encoding", "gzip,deflate,br" },
		{"Accept-Language", "en-US,en;q=0.5"},
		{"Connection", "keep-alive"},
		{"Cache-Control", "no-cache"},
	};
private:
	
};
