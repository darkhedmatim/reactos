/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "hnetcfg_private.h"

typedef struct fw_policy
{
    INetFwPolicy INetFwPolicy_iface;
    LONG refs;
} fw_policy;

static inline fw_policy *impl_from_INetFwPolicy( INetFwPolicy *iface )
{
    return CONTAINING_RECORD(iface, fw_policy, INetFwPolicy_iface);
}

typedef struct fw_policy2
{
    INetFwPolicy2 INetFwPolicy2_iface;
    LONG refs;
} fw_policy2;

static inline fw_policy2 *impl_from_INetFwPolicy2( INetFwPolicy2 *iface )
{
    return CONTAINING_RECORD(iface, fw_policy2, INetFwPolicy2_iface);
}

static ULONG WINAPI fw_policy_AddRef(
    INetFwPolicy *iface )
{
    fw_policy *fw_policy = impl_from_INetFwPolicy( iface );
    return InterlockedIncrement( &fw_policy->refs );
}

static ULONG WINAPI fw_policy_Release(
    INetFwPolicy *iface )
{
    fw_policy *fw_policy = impl_from_INetFwPolicy( iface );
    LONG refs = InterlockedDecrement( &fw_policy->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_policy);
        HeapFree( GetProcessHeap(), 0, fw_policy );
    }
    return refs;
}

static HRESULT WINAPI fw_policy_QueryInterface(
    INetFwPolicy *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_policy *This = impl_from_INetFwPolicy( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwPolicy ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    INetFwPolicy_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_policy_GetTypeInfoCount(
    INetFwPolicy *iface,
    UINT *pctinfo )
{
    fw_policy *This = impl_from_INetFwPolicy( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fw_policy_GetTypeInfo(
    INetFwPolicy *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_policy *This = impl_from_INetFwPolicy( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwPolicy_tid, ppTInfo );
}

static HRESULT WINAPI fw_policy_GetIDsOfNames(
    INetFwPolicy *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_policy *This = impl_from_INetFwPolicy( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwPolicy_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_policy_Invoke(
    INetFwPolicy *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_policy *This = impl_from_INetFwPolicy( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwPolicy_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwPolicy_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_policy_get_CurrentProfile(
    INetFwPolicy *iface,
    INetFwProfile **profile )
{
    fw_policy *This = impl_from_INetFwPolicy( iface );

    TRACE("%p, %p\n", This, profile);
    return NetFwProfile_create( NULL, (void **)profile );
}

static HRESULT WINAPI fw_policy_GetProfileByType(
    INetFwPolicy *iface,
    NET_FW_PROFILE_TYPE profileType,
    INetFwProfile **profile )
{
    fw_policy *This = impl_from_INetFwPolicy( iface );

    FIXME("%p, %u, %p\n", This, profileType, profile);
    return E_NOTIMPL;
}

static const struct INetFwPolicyVtbl fw_policy_vtbl =
{
    fw_policy_QueryInterface,
    fw_policy_AddRef,
    fw_policy_Release,
    fw_policy_GetTypeInfoCount,
    fw_policy_GetTypeInfo,
    fw_policy_GetIDsOfNames,
    fw_policy_Invoke,
    fw_policy_get_CurrentProfile,
    fw_policy_GetProfileByType
};

HRESULT NetFwPolicy_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_policy *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = HeapAlloc( GetProcessHeap(), 0, sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwPolicy_iface.lpVtbl = &fw_policy_vtbl;
    fp->refs = 1;

    *ppObj = &fp->INetFwPolicy_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

static HRESULT WINAPI fwpolicy2_QueryInterface(INetFwPolicy2 *iface, REFIID riid, void **out)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), out );

    if ( IsEqualGUID( riid, &IID_INetFwPolicy2 ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *out = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    INetFwPolicy2_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI fwpolicy2_AddRef(INetFwPolicy2 *iface)
{
    fw_policy2 *fw_policy = impl_from_INetFwPolicy2( iface );
    return InterlockedIncrement( &fw_policy->refs );
}

static ULONG WINAPI fwpolicy2_Release(INetFwPolicy2 *iface)
{
    fw_policy2 *fw_policy = impl_from_INetFwPolicy2( iface );
    LONG refs = InterlockedDecrement( &fw_policy->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_policy);
        HeapFree( GetProcessHeap(), 0, fw_policy );
    }
    return refs;
}

static HRESULT WINAPI fwpolicy2_GetTypeInfoCount(INetFwPolicy2 *iface, UINT *pctinfo)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fwpolicy2_GetTypeInfo(INetFwPolicy2 *iface, UINT iTInfo, LCID lcid, ITypeInfo **info)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, info);
    return get_typeinfo( INetFwPolicy2_tid, info );
}

static HRESULT WINAPI fwpolicy2_GetIDsOfNames(INetFwPolicy2 *iface, REFIID riid, LPOLESTR *rgszNames,
                        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwPolicy2_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fwpolicy2_Invoke(INetFwPolicy2 *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
                WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwPolicy2_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwPolicy2_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fwpolicy2_get_CurrentProfileTypes(INetFwPolicy2 *iface, LONG *profile)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %p\n", This, profile);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_FirewallEnabled(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT_BOOL *enabled)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %p\n", This, profileType, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_put_FirewallEnabled(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT_BOOL enabled)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %d\n", This, profileType, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_ExcludedInterfaces(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT *interfaces)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %p\n", This, profileType, interfaces);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_put_ExcludedInterfaces(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT interfaces)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );
    FIXME("%p %d\n", This, profileType);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_BlockAllInboundTraffic(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT_BOOL *block)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %p\n", This, profileType, block);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_put_BlockAllInboundTraffic(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT_BOOL block)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %d\n", This, profileType, block);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_NotificationsDisabled(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT_BOOL *disabled)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %p\n", This, profileType, disabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_put_NotificationsDisabled(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT_BOOL disabled)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %d\n", This, profileType, disabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_UnicastResponsesToMulticastBroadcastDisabled(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT_BOOL *disabled)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %p\n", This, profileType, disabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_put_UnicastResponsesToMulticastBroadcastDisabled(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, VARIANT_BOOL disabled)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %d\n", This, profileType, disabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_Rules(INetFwPolicy2 *iface, INetFwRules **rules)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %p\n", This, rules);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_ServiceRestriction(INetFwPolicy2 *iface, INetFwServiceRestriction **ServiceRestriction)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %p\n", This, ServiceRestriction);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_EnableRuleGroup(INetFwPolicy2 *iface, LONG bitmask, BSTR group, VARIANT_BOOL enable)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %s %d\n", This, bitmask, debugstr_w(group), enable);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_IsRuleGroupEnabled(INetFwPolicy2 *iface, LONG bitmask, BSTR group, VARIANT_BOOL *enabled)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %s %p\n", This, bitmask, debugstr_w(group), enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_RestoreLocalFirewallDefaults(INetFwPolicy2 *iface)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_DefaultInboundAction(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, NET_FW_ACTION *action)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %p\n", This, profileType, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_put_DefaultInboundAction(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, NET_FW_ACTION action)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %d\n", This, profileType, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_DefaultOutboundAction(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, NET_FW_ACTION *action)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %p\n", This, profileType, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_put_DefaultOutboundAction(INetFwPolicy2 *iface, NET_FW_PROFILE_TYPE2 profileType, NET_FW_ACTION action)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %d %d\n", This, profileType, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_IsRuleGroupCurrentlyEnabled(INetFwPolicy2 *iface, BSTR group, VARIANT_BOOL *enabled)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %s %p\n", This, debugstr_w(group), enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI fwpolicy2_get_LocalPolicyModifyState(INetFwPolicy2 *iface, NET_FW_MODIFY_STATE *modifyState)
{
    fw_policy2 *This = impl_from_INetFwPolicy2( iface );

    FIXME("%p %p\n", This, modifyState);
    return E_NOTIMPL;
}

static const struct INetFwPolicy2Vtbl fw_policy2_vtbl =
{
    fwpolicy2_QueryInterface,
    fwpolicy2_AddRef,
    fwpolicy2_Release,
    fwpolicy2_GetTypeInfoCount,
    fwpolicy2_GetTypeInfo,
    fwpolicy2_GetIDsOfNames,
    fwpolicy2_Invoke,
    fwpolicy2_get_CurrentProfileTypes,
    fwpolicy2_get_FirewallEnabled,
    fwpolicy2_put_FirewallEnabled,
    fwpolicy2_get_ExcludedInterfaces,
    fwpolicy2_put_ExcludedInterfaces,
    fwpolicy2_get_BlockAllInboundTraffic,
    fwpolicy2_put_BlockAllInboundTraffic,
    fwpolicy2_get_NotificationsDisabled,
    fwpolicy2_put_NotificationsDisabled,
    fwpolicy2_get_UnicastResponsesToMulticastBroadcastDisabled,
    fwpolicy2_put_UnicastResponsesToMulticastBroadcastDisabled,
    fwpolicy2_get_Rules,
    fwpolicy2_get_ServiceRestriction,
    fwpolicy2_EnableRuleGroup,
    fwpolicy2_IsRuleGroupEnabled,
    fwpolicy2_RestoreLocalFirewallDefaults,
    fwpolicy2_get_DefaultInboundAction,
    fwpolicy2_put_DefaultInboundAction,
    fwpolicy2_get_DefaultOutboundAction,
    fwpolicy2_put_DefaultOutboundAction,
    fwpolicy2_get_IsRuleGroupCurrentlyEnabled,
    fwpolicy2_get_LocalPolicyModifyState
};

HRESULT NetFwPolicy2_create( IUnknown *outer, void **obj )
{
    fw_policy2 *fp;

    TRACE("(%p,%p)\n", outer, obj);

    fp = HeapAlloc( GetProcessHeap(), 0, sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwPolicy2_iface.lpVtbl = &fw_policy2_vtbl;
    fp->refs = 1;

    *obj = &fp->INetFwPolicy2_iface;

    TRACE("returning iface %p\n", *obj);
    return S_OK;
}
