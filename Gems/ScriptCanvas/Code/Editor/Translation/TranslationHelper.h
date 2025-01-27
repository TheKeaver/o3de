/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QCoreApplication>

#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/conversions.h>

#include <ScriptCanvas/Core/Slot.h>
#include <GraphCanvas/Types/TranslationTypes.h>
#include <Source/Translation/TranslationBus.h>
#include <AzFramework/Gem/GemInfo.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace Translation
{
    namespace GlobalKeys
    {
        static constexpr const char* EBusSenderIDKey = "Globals.EBusSenderBusId";
        static constexpr const char* EBusHandlerIDKey = "Globals.EBusHandlerBusId";
        static constexpr const char* MissingFunctionKey = "Globals.MissingFunction";
        static constexpr const char* EBusHandlerOutSlot = "Globals.EBusHandler.OutSlot";
    }

    static inline bool GetValue(const AZStd::string key, AZStd::string& value)
    {
        GraphCanvas::TranslationKey tkey;
        tkey = key;

        bool result = false;
        GraphCanvas::TranslationRequestBus::BroadcastResult(result, &GraphCanvas::TranslationRequests::Get, key, value);
        return result;
    }
}


namespace GraphCanvasAttributeHelper
{
    template <typename T>
    AZStd::string GetStringAttribute(const T* source, const AZ::Crc32& attribute)
    {
        AZStd::string attributeValue = "";
        if (auto attributeItem = azrtti_cast<AZ::AttributeData<AZStd::string>*>(AZ::FindAttribute(attribute, source->m_attributes)))
        {
            attributeValue = attributeItem->Get(nullptr);
        }
        return attributeValue;
    }

    inline AZStd::string ReadStringAttribute(const AZ::AttributeArray& attributes, const AZ::Crc32& attribute)
    {
        AZStd::string attributeValue = "";
        if (auto attributeItem = azrtti_cast<AZ::AttributeData<AZStd::string>*>(AZ::FindAttribute(attribute, attributes)))
        {
            attributeValue = attributeItem->Get(nullptr);
            return attributeValue;
        }

        if (auto attributeItem = azrtti_cast<AZ::AttributeData<const char*>*>(AZ::FindAttribute(attribute, attributes)))
        {
            attributeValue = attributeItem->Get(nullptr);
            return attributeValue;
        }

        return {};
    }
}

namespace ScriptCanvasEditor
{
    enum class TranslationContextGroup : AZ::u32
    {
        EbusSender,
        EbusHandler,
        ClassMethod,
        GlobalMethod,
        Invalid
    };

    enum class TranslationItemType : AZ::u32
    {
        Node,
        Wrapper,
        ExecutionInSlot,
        ExecutionOutSlot,
        ParamDataSlot,
        ReturnDataSlot,
        BusIdSlot,
        Invalid
    };

    enum class TranslationKeyId : AZ::u32
    {
        Name,
        Tooltip,
        Category,
        Invalid
    };


    namespace TranslationKeyParts
    {
        const char* const handler = "HANDLER_";
        const char* const name = "NAME";
        const char* const tooltip = "TOOLTIP";
        const char* const category = "CATEGORY";
        const char* const in = "IN";
        const char* const out = "OUT";
        const char* const param = "PARAM";
        const char* const output = "OUTPUT";
        const char* const busid = "BUSID";
    }

    namespace TranslationContextGroupParts
    {
        const char* const ebusSender  = "EBus";
        const char* const ebusHandler = "Handler";
        const char* const classMethod = "Method";
        constexpr const char* const globalMethod = "GlobalMethod";
    };

    // The context name and keys generated by TranslationHelper should match the keys
    // being exported by the TSGenerateAction.cpp in the ScriptCanvasDeveloper Gem.
    class TranslationHelper
    {
    public:
        static AZStd::string GetContextName(TranslationContextGroup group, AZStd::string_view keyBase)
        {
            if (group == TranslationContextGroup::Invalid || keyBase.empty())
            {
                // Missing information
                return AZStd::string();
            }

            const char* groupPart;

            switch (group)
            {
            case TranslationContextGroup::EbusSender:
                groupPart = TranslationContextGroupParts::ebusSender;
                break;
            case TranslationContextGroup::EbusHandler:
                groupPart = TranslationContextGroupParts::ebusHandler;
                break;
            case TranslationContextGroup::ClassMethod:
                groupPart = TranslationContextGroupParts::classMethod;
                break;
            case TranslationContextGroup::GlobalMethod:
                groupPart = TranslationContextGroupParts::globalMethod;
                break;
            default:
                AZ_Warning("TranslationComponent", false, "Invalid translation group ID.");
                groupPart = "";
            }

            AZStd::string fullKey = AZStd::string::format("%s: %.*s", groupPart,
                aznumeric_cast<int>(keyBase.size()), keyBase.data());

            return fullKey;
        }
        
        // UserDefined
        static AZStd::string GetUserDefinedNodeKey(AZStd::string_view contextName, AZStd::string_view nodeName, TranslationKeyId keyId)
        {
            return GetKey(TranslationContextGroup::ClassMethod, contextName, nodeName, TranslationItemType::Node, keyId);
        }
        ////

        static AZStd::string GetKey(TranslationContextGroup group, AZStd::string_view keyBase, AZStd::string_view keyName, TranslationItemType type, TranslationKeyId keyId, int paramIndex = 0)
        {
            if (group == TranslationContextGroup::Invalid || keyBase.empty()
                || type == TranslationItemType::Invalid || keyId == TranslationKeyId::Invalid)
            {
                // Missing information
                return AZStd::string();
            }

            if (type != TranslationItemType::Wrapper && keyName.empty())
            {
                // Missing information
                return AZStd::string();
            }

            AZStd::string fullKey;

            const char* prefix = "";
            if (group == TranslationContextGroup::EbusHandler)
            {
                prefix = TranslationKeyParts::handler;
            }

            const char* keyPart = GetKeyPart(keyId);

            switch (type)
            {
            case TranslationItemType::Node:
                fullKey = AZStd::string::format("%s%.*s_%.*s_%s",
                    prefix,
                    aznumeric_cast<int>(keyBase.size()),
                    keyBase.data(),
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    keyPart
                );
                break;
            case TranslationItemType::Wrapper:
                fullKey = GetClassKey(group, keyBase, keyId);
                break;
            case TranslationItemType::ExecutionInSlot:
                fullKey = AZStd::string::format("%s%.*s_%.*s_%s_%s",
                    prefix,
                    aznumeric_cast<int>(keyBase.size()),
                    keyBase.data(),
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::in,
                    keyPart
                );
                break;
            case TranslationItemType::ExecutionOutSlot:
                fullKey = AZStd::string::format("%s%.*s_%.*s_%s_%s",
                    prefix,
                    aznumeric_cast<int>(keyBase.size()),
                    keyBase.data(),
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::out,
                    keyPart
                );
                break;
            case TranslationItemType::ParamDataSlot:
                fullKey = AZStd::string::format("%s%.*s_%.*s_%s%d_%s",
                    prefix,
                    aznumeric_cast<int>(keyBase.size()),
                    keyBase.data(),
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::param,
                    paramIndex,
                    keyPart
                );
                break;
            case TranslationItemType::ReturnDataSlot:
                fullKey = AZStd::string::format("%s%.*s_%.*s_%s%d_%s",
                    prefix,
                    aznumeric_cast<int>(keyBase.size()),
                    keyBase.data(),
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::output,
                    paramIndex,
                    keyPart
                );
                break;
            case TranslationItemType::BusIdSlot:
                fullKey = AZStd::string::format("%s%.*s_%.*s_%s_%s",
                    prefix,
                    aznumeric_cast<int>(keyBase.size()),
                    keyBase.data(),
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::busid,
                    keyPart
                );
                break;
            default:
                AZ_Warning("ScriptCanvas TranslationHelper", false, "Invalid translation item type.");
            }

            AZStd::to_upper(fullKey.begin(), fullKey.end());

            return fullKey;
        }

        static AZStd::string GetClassKey(TranslationContextGroup group, AZStd::string_view keyBase, TranslationKeyId keyId)
        {
            const char* prefix = "";
            if (group == TranslationContextGroup::EbusHandler)
            {
                prefix = TranslationKeyParts::handler;
            }

            const char* keyPart = GetKeyPart(keyId);

            AZStd::string fullKey = AZStd::string::format("%s%.*s_%s",
                prefix,
                aznumeric_cast<int>(keyBase.size()),
                keyBase.data(),
                keyPart
            );

            AZStd::to_upper(fullKey.begin(), fullKey.end());

            return fullKey;
        }

        static AZStd::string GetGlobalMethodKey(AZStd::string_view keyName, TranslationItemType keyType,
            TranslationKeyId keyId, int paramIndex = 0)
        {
            const char* keyPart = GetKeyPart(keyId);

            AZStd::string fullKey;
            switch (keyType)
            {
            case TranslationItemType::Node:
                fullKey = AZStd::string::format("%.*s_%s",
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    keyPart
                );
                break;
            case TranslationItemType::ExecutionInSlot:
                fullKey = AZStd::string::format("%.*s_%s_%s",
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::in,
                    keyPart
                );
                break;
            case TranslationItemType::ExecutionOutSlot:
                fullKey = AZStd::string::format("%.*s_%s_%s",
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::out,
                    keyPart
                );
                break;
            case TranslationItemType::ParamDataSlot:
                fullKey = AZStd::string::format("%.*s_%s%d_%s",
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::param,
                    paramIndex,
                    keyPart
                );
                break;
            case TranslationItemType::ReturnDataSlot:
                fullKey = AZStd::string::format("%.*s_%s%d_%s",
                    aznumeric_cast<int>(keyName.size()),
                    keyName.data(),
                    TranslationKeyParts::output,
                    paramIndex,
                    keyPart
                );
                break;
            default:
                AZ_Warning("ScriptCanvas TranslationHelper", false, "Invalid translation item type.");
            }

            AZStd::to_upper(fullKey.begin(), fullKey.end());

            return fullKey;
        }

        static const char* GetKeyPart(TranslationKeyId keyId)
        {
            const char* keyPart = "";

            switch (keyId)
            {
            case TranslationKeyId::Name:
                keyPart = TranslationKeyParts::name;
                break;
            case TranslationKeyId::Tooltip:
                keyPart = TranslationKeyParts::tooltip;
                break;
            case TranslationKeyId::Category:
                keyPart = TranslationKeyParts::category;
                break;


            default:
                AZ_Warning("ScriptCanvas TranslationHelper", false, "Invalid translation key ID.");
            }

            return keyPart;
        }

        static TranslationItemType GetItemType(ScriptCanvas::SlotDescriptor slotDescriptor)
        {
            if (slotDescriptor == ScriptCanvas::SlotDescriptors::ExecutionIn())
            {
                return TranslationItemType::ExecutionInSlot;
            }
            else if (slotDescriptor == ScriptCanvas::SlotDescriptors::ExecutionOut())
            {
                return TranslationItemType::ExecutionOutSlot;
            }
            else if (slotDescriptor == ScriptCanvas::SlotDescriptors::DataIn())
            {
                return TranslationItemType::ParamDataSlot;
            }
            else if (slotDescriptor == ScriptCanvas::SlotDescriptors::DataOut())
            {
                return TranslationItemType::ReturnDataSlot;
            }

            return TranslationItemType::Invalid;
        }

        static AZStd::string GetSafeTypeName(ScriptCanvas::Data::Type dataType)
        {
            if (!dataType.IsValid())
            {
                return "";
            }

            return ScriptCanvas::Data::GetName(dataType);
        }

        static AZStd::string GetKeyTranslation(TranslationContextGroup group, AZStd::string_view keyBase, AZStd::string_view keyName, TranslationItemType type, TranslationKeyId keyId, int paramIndex = 0)
        {
            AZStd::string translationContext = TranslationHelper::GetContextName(group, keyBase);
            AZStd::string translationKey = TranslationHelper::GetKey(group, keyBase, keyName, type, keyId, paramIndex);
            AZStd::string translated = QCoreApplication::translate(translationContext.c_str(), translationKey.c_str()).toUtf8().data();

            if (translated == translationKey)
            {
                return AZStd::string();
            }

            return translated;
        }

        static AZStd::string GetClassKeyTranslation(TranslationContextGroup group, AZStd::string_view keyBase, TranslationKeyId keyId)
        {
            AZStd::string translationContext = TranslationHelper::GetContextName(group, keyBase);
            AZStd::string translationKey = TranslationHelper::GetClassKey(group, keyBase, keyId);
            AZStd::string translated = QCoreApplication::translate(translationContext.c_str(), translationKey.c_str()).toUtf8().data();

            if (translated == translationKey)
            {
                return AZStd::string();
            }

            return translated;
        }

        static AZStd::string GetGlobalMethodKeyTranslation(AZStd::string_view keyName,
            TranslationItemType keyType, TranslationKeyId keyId, int paramIndex = 0)
        {
            AZStd::string translationKey = TranslationHelper::GetGlobalMethodKey(keyName, keyType, keyId, paramIndex);
            AZStd::string translated = QCoreApplication::translate(TranslationContextGroupParts::globalMethod, translationKey.c_str()).toUtf8().data();

            if (translated == translationKey)
            {
                return AZStd::string();
            }

            return translated;
        }

        // Use the StackedString to index the translation keys as a Json Pointer
        static AZ::StackedString GetAzEventHandlerRootPointer(AZStd::string_view eventName)
        {
            AZ::StackedString path(AZ::StackedString::Format::JsonPointer);
            path.Push(eventName);

            return path;
        }

        

    };


}
