// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ClientConfig.h"
#include "Content.h"
#include "Logging.h"
#include "RequestParams.h"
#include "Result.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace SFS
{
class ApplicabilityDetails;
class DeliveryOptimizationData;

namespace details
{
class SFSClientInterface;
}

class SFSClient
{
  public:
    ~SFSClient() noexcept;

    SFSClient(const SFSClient&) = delete;
    SFSClient& operator=(const SFSClient&) = delete;

    /**
     * @brief Make a new SFSClient object
     * @details An SFSClient object is used to make calls to the SFS service. The SFSClient object is initialized with
     * a few parameters that are used to build the URL for the SFS service. The URL is built as follows:
     * https://{accountId}.api.cdp.microsoft.com/api/v2/contents/{instanceId}/namespaces/{nameSpace}
     * The instanceId and nameSpace are optionally set in @param config and have a default value if not provided.
     * The accountId is required and must be set to a non-empty value.
     *
     * @param config Describes a set of startup configurations for the SFSClient
     */
    [[nodiscard]] static Result Make(ClientConfig config, std::unique_ptr<SFSClient>& out) noexcept;

    //
    // API to retrieve download information from the SFS Service
    //

    /**
     * @brief Retrieve combined metadata & download URLs from the latest version of specified products
     * @note At the moment only a single product request is supported
     * @param requestParams Parameters that define this request
     * @param content A pointer to a Content instance that is populated with the result
     */
    [[nodiscard]] Result GetLatestDownloadInfo(const RequestParams& requestParams,
                                               std::unique_ptr<Content>& content) const noexcept;

    //
    // API to retrieve optional extra download information from the SFS Service
    //

    /**
     * @brief Retrieve Delivery Optimization data for a given piece of content, if existing
     * @details If the Delivery Optimization data is not available, the result code will be set to Result::NotSet
     * and the param data will not be modified
     * @param content A content object that was returned from a previous call to GetDownloadInfo
     * @param data A DeliveryOptimizationData object that is populated with the result
     */
    [[nodiscard]] Result GetDeliveryOptimizationData(const Content& content,
                                                     std::unique_ptr<DeliveryOptimizationData>& data) const noexcept;

    /**
     * @brief Retrieve Applicability details for a given piece of content, if existing
     * @details If the Applicability details are not available, the result code will be set to Result::NotSet
     * and the param details will not be modified
     * @param content A content object that was returned from a previous call to GetDownloadInfo
     * @param data A ApplicabilityDetails object that is populated with the result
     */
    [[nodiscard]] Result GetApplicabilityDetails(const Content& content,
                                                 std::unique_ptr<ApplicabilityDetails>& details) const noexcept;

  private:
    /**
     * @brief Construct a new SFSClient object
     */
    SFSClient() noexcept;

    std::unique_ptr<details::SFSClientInterface> m_impl;
};
} // namespace SFS
