// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentUtil.h"

#include "ErrorHandling.h"
#include "ReportingHandler.h"
#include "Util.h"

#include <nlohmann/json.hpp>

using namespace SFS;
using namespace SFS::details;
using namespace SFS::details::util;
using json = nlohmann::json;

#define THROW_INVALID_RESPONSE_IF_NOT(condition, message, handler)                                                     \
    THROW_CODE_IF_NOT_LOG(ServiceInvalidResponse, condition, handler, message)

namespace
{
HashType HashTypeFromString(const std::string& hashType, const ReportingHandler& handler)
{
    if (AreEqualI(hashType, "Sha1"))
    {
        return HashType::Sha1;
    }
    else if (AreEqualI(hashType, "Sha256"))
    {
        return HashType::Sha256;
    }
    else
    {
        THROW_LOG(Result(Result::Unexpected, "Unknown hash type: " + hashType), handler);
        return HashType::Sha1; // Unreachable code, but the compiler doesn't know that.
    }
}

void ValidateContentType(const VersionEntity& versionEntity, ContentType expectedType, const ReportingHandler& handler)
{
    THROW_CODE_IF_LOG(Result::ServiceInvalidResponse,
                      versionEntity.GetContentType() != expectedType,
                      handler,
                      "Unexpected content type returned by the service");
}
} // namespace

std::unique_ptr<VersionEntity> contentutil::ParseJsonToVersionEntity(const nlohmann::json& data,
                                                                     const ReportingHandler& handler)
{
    // Expected format for a generic version entity:
    // {
    //   "ContentId": {
    //     "Namespace": <ns>,
    //     "Name": <name>,
    //     "Version": <version>
    //   }
    // }
    //

    // TODO #50: Use a different entity once the service supports app content.

    std::unique_ptr<GenericVersionEntity> tmp = std::make_unique<GenericVersionEntity>();

    THROW_INVALID_RESPONSE_IF_NOT(data.is_object(), "Response is not a JSON object", handler);
    THROW_INVALID_RESPONSE_IF_NOT(data.contains("ContentId"), "Missing ContentId in response", handler);

    const auto& contentId = data["ContentId"];
    THROW_INVALID_RESPONSE_IF_NOT(contentId.is_object(), "ContentId is not a JSON object", handler);

    THROW_INVALID_RESPONSE_IF_NOT(contentId.contains("Namespace"), "Missing ContentId.Namespace in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(contentId["Namespace"].is_string(), "ContentId.Namespace is not a string", handler);
    tmp->contentId.nameSpace = contentId["Namespace"];

    THROW_INVALID_RESPONSE_IF_NOT(contentId.contains("Name"), "Missing ContentId.Name in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(contentId["Name"].is_string(), "ContentId.Name is not a string", handler);
    tmp->contentId.name = contentId["Name"];

    THROW_INVALID_RESPONSE_IF_NOT(contentId.contains("Version"), "Missing ContentId.Version in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(contentId["Version"].is_string(), "ContentId.Version is not a string", handler);
    tmp->contentId.version = contentId["Version"];

    return tmp;
}

std::unique_ptr<ContentId> contentutil::GenericVersionEntityToContentId(VersionEntity&& entity,
                                                                        const ReportingHandler& handler)
{
    ValidateContentType(entity, ContentType::Generic, handler);

    std::unique_ptr<ContentId> tmp;
    THROW_IF_FAILED_LOG(ContentId::Make(std::move(entity.contentId.nameSpace),
                                        std::move(entity.contentId.name),
                                        std::move(entity.contentId.version),
                                        tmp),
                        handler);
    return tmp;
}

std::unique_ptr<File> contentutil::FileJsonToObj(const json& file, const ReportingHandler& handler)
{
    THROW_INVALID_RESPONSE_IF_NOT(file.is_object(), "File is not a JSON object", handler);

    THROW_INVALID_RESPONSE_IF_NOT(file.contains("FileId"), "Missing File.FileId in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(file["FileId"].is_string(), "File.FileId is not a string", handler);
    std::string fileId = file["FileId"];

    THROW_INVALID_RESPONSE_IF_NOT(file.contains("Url"), "Missing File.Url in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(file["Url"].is_string(), "File.Url is not a string", handler);
    std::string url = file["Url"];

    THROW_INVALID_RESPONSE_IF_NOT(file.contains("SizeInBytes"), "Missing File.SizeInBytes in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(file["SizeInBytes"].is_number_unsigned(),
                                  "File.SizeInBytes is not an unsigned number",
                                  handler);
    uint64_t sizeInBytes = file["SizeInBytes"];

    THROW_INVALID_RESPONSE_IF_NOT(file.contains("Hashes"), "Missing File.Hashes in response", handler);
    THROW_INVALID_RESPONSE_IF_NOT(file["Hashes"].is_object(), "File.Hashes is not an object", handler);
    std::unordered_map<HashType, std::string> hashes;
    for (const auto& [hashType, hashValue] : file["Hashes"].items())
    {
        THROW_INVALID_RESPONSE_IF_NOT(hashValue.is_string(), "File.Hashes object value is not a string", handler);
        hashes[HashTypeFromString(hashType, handler)] = hashValue;
    }

    std::unique_ptr<File> tmp;
    THROW_IF_FAILED_LOG(File::Make(std::move(fileId), std::move(url), sizeInBytes, std::move(hashes), tmp), handler);

    return tmp;
}

bool contentutil::operator==(const ContentId& lhs, const ContentId& rhs)
{
    // String characters can be UTF-8 encoded, so we need to compare them in a case-sensitive manner.
    return lhs.GetNameSpace() == rhs.GetNameSpace() && lhs.GetName() == rhs.GetName() &&
           lhs.GetVersion() == rhs.GetVersion();
}

bool contentutil::operator!=(const ContentId& lhs, const ContentId& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const File& lhs, const File& rhs)
{
    // String characters can be UTF-8 encoded, so we need to compare them in a case-sensitive manner.
    return lhs.GetFileId() == rhs.GetFileId() && lhs.GetUrl() == rhs.GetUrl() &&
           lhs.GetSizeInBytes() == rhs.GetSizeInBytes() && lhs.GetHashes() == rhs.GetHashes();
}

bool contentutil::operator!=(const File& lhs, const File& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const ApplicabilityDetails& lhs, const ApplicabilityDetails& rhs)
{
    // String characters can be UTF-8 encoded, so we need to compare them in a case-sensitive manner.
    return lhs.GetArchitectures() == rhs.GetArchitectures() &&
           lhs.GetPlatformApplicabilityForPackage() == rhs.GetPlatformApplicabilityForPackage();
}

bool contentutil::operator!=(const ApplicabilityDetails& lhs, const ApplicabilityDetails& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const AppFile& lhs, const AppFile& rhs)
{
    // String characters can be UTF-8 encoded, so we need to compare them in a case-sensitive manner.
    return lhs.GetFileId() == rhs.GetFileId() && lhs.GetUrl() == rhs.GetUrl() &&
           lhs.GetSizeInBytes() == rhs.GetSizeInBytes() && lhs.GetHashes() == rhs.GetHashes() &&
           lhs.GetApplicabilityDetails() == rhs.GetApplicabilityDetails() &&
           lhs.GetFileMoniker() == rhs.GetFileMoniker();
}

bool contentutil::operator!=(const AppFile& lhs, const AppFile& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const Content& lhs, const Content& rhs)
{
    return lhs.GetContentId() == rhs.GetContentId() &&
           (std::is_permutation(lhs.GetFiles().begin(),
                                lhs.GetFiles().end(),
                                rhs.GetFiles().begin(),
                                rhs.GetFiles().end(),
                                [](const File& flhs, const File& frhs) { return flhs == frhs; }));
}

bool contentutil::operator!=(const Content& lhs, const Content& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const AppPrerequisiteContent& lhs, const AppPrerequisiteContent& rhs)
{
    auto areFilesEqual = [&lhs, &rhs]() {
        return std::is_permutation(lhs.GetFiles().begin(),
                                   lhs.GetFiles().end(),
                                   rhs.GetFiles().begin(),
                                   rhs.GetFiles().end(),
                                   [](const AppFile& flhs, const AppFile& frhs) { return flhs == frhs; });
    };
    return lhs.GetContentId() == rhs.GetContentId() && areFilesEqual();
}

bool contentutil::operator!=(const AppPrerequisiteContent& lhs, const AppPrerequisiteContent& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const AppContent& lhs, const AppContent& rhs)
{
    auto arePrerequisitesEqual = [&lhs, &rhs]() {
        return std::equal(
            lhs.GetPrerequisites().begin(),
            lhs.GetPrerequisites().end(),
            rhs.GetPrerequisites().begin(),
            rhs.GetPrerequisites().end(),
            [](const AppPrerequisiteContent& clhs, const AppPrerequisiteContent& crhs) { return clhs == crhs; });
    };
    auto areFilesEqual = [&lhs, &rhs]() {
        return std::is_permutation(lhs.GetFiles().begin(),
                                   lhs.GetFiles().end(),
                                   rhs.GetFiles().begin(),
                                   rhs.GetFiles().end(),
                                   [](const AppFile& flhs, const AppFile& frhs) { return flhs == frhs; });
    };
    return lhs.GetContentId() == rhs.GetContentId() && lhs.GetUpdateId() == rhs.GetUpdateId() &&
           arePrerequisitesEqual() && areFilesEqual();
}

bool contentutil::operator!=(const AppContent& lhs, const AppContent& rhs)
{
    return !(lhs == rhs);
}
