#pragma once
#include "HttpServerRequest.h"
#include "TinyHttpTypes.h"

struct FHttpServerResponse;

struct TINYHTTP_API FTinyHttp
{
	/**
	 * Convert a UTF-8 payload to a TCHAR payload.
	 * @param InUTF8Payload The UTF-8 payload in binary format.
	 * @param OutTCHARPayload The converted TCHAR output in binary format.
	 */
	static void ConvertToTCHAR(TConstArrayView<uint8> InUTF8Payload, TArray<uint8>& OutTCHARPayload);
	static void ConvertToString(TConstArrayView<uint8> InUTF8Payload, FString& OutString);

	/**
	 * Convert a TCHAR payload to UTF-8.
	 * @param InTCHARPayload The TCHAR payload in binary format.
	 * @param OutUTF8Payload The converted UTF-8 output in binary format.
	 */
	static void ConvertToUTF8(TConstArrayView<uint8> InTCHARPayload, TArray<uint8>& OutUTF8Payload);

	/**
	 * Convert a FString to UTF-8.
	 * @param InString The string to be converted.
	 * @param OutUTF8Payload the converted UTF-8 output in binary format.
	 */
	static void ConvertToUTF8(const FString& InString, TArray<uint8>& OutUTF8Payload);

	/** Json UTF8 <-> UStruct */
	static TSharedPtr<FJsonObject> JsonPayloadToObject(const TArray<uint8>& InUTF8Payload);
	static bool JsonPayloadToUStruct(const TArray<uint8>& InUTF8Payload, const UStruct* StructDefinition, void* OutStruct);
	static bool UStructToJsonPayload(TArray<uint8>& OutUTF8Payload, const UStruct* StructDefinition, const void* Struct);

	/**
	 * Deserialize a request into a UStruct.
	 * @param InRequest The incoming http request.
	 * @param OutDeserializedRequest The structure to serialize using the request's content.
	 * @return Whether the deserialization was successful. 
	 * 
	 * @note InCompleteCallback will be called with an appropriate http response if the deserialization fails.
	 */
	template <typename RequestType>
	static bool DeserializeRequest(const FHttpServerRequest& InRequest, RequestType& OutDeserializedRequest)
	{
		return JsonPayloadToUStruct(InRequest.Body, RequestType::StaticStruct(), &OutDeserializedRequest);
	}

	/**
	 * Serialize a response object into a UTF-8 Payload.
	 * @param InResponseObject the object to serialize.
	 * @param OutResponsePayload the resulting UTF-8 payload.
	 */
	template <typename ResponseType>
	static bool SerializeResponse(const ResponseType& InResponseObject, TArray<uint8>& OutResponsePayload)
	{
		return UStructToJsonPayload(OutResponsePayload, ResponseType::StaticStruct(), &InResponseObject);
	}

	static FString RequestVerbToString(EHttpServerRequestVerbs Verb);
	static FString RequestBodyToString(const FHttpServerRequest& Request);
	static FString RequestToDebugString(const FHttpServerRequest& Request, bool bShowBody = true);

	/**
	 * Service Response
	 */
	template <typename ServiceResponseStruct>
	static TUniquePtr<FHttpServerResponse> ServiceResponse(const TSharedPtr<ServiceResponseStruct>& Reponse)
	{
		return ServiceResponseInternal(Reponse, ServiceResponseStruct::StaticStruct());
	}

	static TUniquePtr<FHttpServerResponse> ServiceResponse(const FServiceResponsePtr& Reponse, const UStruct* ResponseStruct)
	{
		return ServiceResponseInternal(Reponse, ResponseStruct);
	}

	static TUniquePtr<FHttpServerResponse> ServiceOK()
	{
		return ServiceOKInternal(nullptr, nullptr);
	}

	template <typename ResponseDataStruct>
	static TUniquePtr<FHttpServerResponse> ServiceOK(const ResponseDataStruct& Data)
	{
		return ServiceOKInternal(ResponseDataStruct::StaticStruct(), &Data);
	}

	static TUniquePtr<FHttpServerResponse> ServiceError(const int ErrorCode, const FString& ErrorMessage)
	{
		return ServiceErrorInternal(ErrorCode, ErrorMessage, nullptr, nullptr);
	}

	template <typename ErrorDetailStruct>
	static TUniquePtr<FHttpServerResponse> ServiceError(const int ErrorCode, const FString& ErrorMessage, const ErrorDetailStruct& ErrorDetail)
	{
		return ServiceErrorInternal(ErrorCode, ErrorMessage, ErrorDetailStruct::StaticStruct(), &ErrorDetail);
	}

	/**
	 * Client deserialize the service's response
	 */
	

protected:
	static TUniquePtr<FHttpServerResponse> ServiceResponseInternal(const FServiceResponsePtr& ServiceResponse, const UStruct* ServiceResponseStruct);
	static TUniquePtr<FHttpServerResponse> ServiceOKInternal(const UStruct* StructDefinition, const void* Struct);
	static TUniquePtr<FHttpServerResponse> ServiceErrorInternal(const int ErrorCode, const FString& ErrorMessage, const UStruct* StructDefinition, const void* Struct);
};
