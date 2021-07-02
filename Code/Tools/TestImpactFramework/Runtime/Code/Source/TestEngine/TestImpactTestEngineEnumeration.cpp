/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/TestImpactTestEngineEnumeration.h>

namespace TestImpact
{
    TestEngineEnumeration::TestEngineEnumeration(TestEngineJob&& job, AZStd::optional<TestEnumeration>&& enumeration)
        : TestEngineJob(AZStd::move(job))
        , m_enumeration(AZStd::move(enumeration))
    {
    }

    const AZStd::optional<TestEnumeration>& TestEngineEnumeration::GetTestEnumeration() const
    {
        return m_enumeration;
    }
} // namespace TestImpact