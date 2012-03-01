
if(NOT DEFINED I18N_LANG)
    set(I18N_LANG all)
endif()

function(set_i18n_language I18N_LANG)
    if(I18N_LANG STREQUAL "af-ZA")
        set(I18N_DEFS -DLANGUAGE_AF_ZA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-AE")
        set(I18N_DEFS -DLANGUAGE_AR_AE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-BH")
        set(I18N_DEFS -DLANGUAGE_AR_BH PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-DZ")
        set(I18N_DEFS -DLANGUAGE_AR_DZ PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-EG")
        set(I18N_DEFS -DLANGUAGE_AR_EG PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-IQ")
        set(I18N_DEFS -DLANGUAGE_AR_IQ PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-JO")
        set(I18N_DEFS -DLANGUAGE_AR_JO PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-KW")
        set(I18N_DEFS -DLANGUAGE_AR_KW PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-LB")
        set(I18N_DEFS -DLANGUAGE_AR_LB PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-LY")
        set(I18N_DEFS -DLANGUAGE_AR_LY PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-MA")
        set(I18N_DEFS -DLANGUAGE_AR_MA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-OM")
        set(I18N_DEFS -DLANGUAGE_AR_OM PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-QA")
        set(I18N_DEFS -DLANGUAGE_AR_QA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-SA")
        set(I18N_DEFS -DLANGUAGE_AR_SA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-SY")
        set(I18N_DEFS -DLANGUAGE_AR_SY PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-TN")
        set(I18N_DEFS -DLANGUAGE_AR_TN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ar-YE")
        set(I18N_DEFS -DLANGUAGE_AR_YE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "az-AZ")
        set(I18N_DEFS -DLANGUAGE_AZ_AZ PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "be-BY")
        set(I18N_DEFS -DLANGUAGE_BE_BY PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "bg-BG")
        set(I18N_DEFS -DLANGUAGE_BG_BG PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ca-ES")
        set(I18N_DEFS -DLANGUAGE_CA_ES PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "cs-CZ")
        set(I18N_DEFS -DLANGUAGE_CS_CZ PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "da-DK")
        set(I18N_DEFS -DLANGUAGE_DA_DK PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "de-AT")
        set(I18N_DEFS -DLANGUAGE_DE_AT PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "de-CH")
        set(I18N_DEFS -DLANGUAGE_DE_CH PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "de-DE")
        set(I18N_DEFS -DLANGUAGE_DE_DE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "de-LI")
        set(I18N_DEFS -DLANGUAGE_DE_LI PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "de-LU")
        set(I18N_DEFS -DLANGUAGE_DE_LU PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "dv-MV")
        set(I18N_DEFS -DLANGUAGE_DV_MV PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "el-GR")
        set(I18N_DEFS -DLANGUAGE_EL_GR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-AU")
        set(I18N_DEFS -DLANGUAGE_EN_AU PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-BZ")
        set(I18N_DEFS -DLANGUAGE_EN_BZ PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-CA")
        set(I18N_DEFS -DLANGUAGE_EN_CA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-CB")
        set(I18N_DEFS -DLANGUAGE_EN_CB PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-GB")
        set(I18N_DEFS -DLANGUAGE_EN_GB PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-IE")
        set(I18N_DEFS -DLANGUAGE_EN_IE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-JM")
        set(I18N_DEFS -DLANGUAGE_EN_JM PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-NZ")
        set(I18N_DEFS -DLANGUAGE_EN_NZ PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-PH")
        set(I18N_DEFS -DLANGUAGE_EN_PH PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-TT")
        set(I18N_DEFS -DLANGUAGE_EN_TT PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-US")
        set(I18N_DEFS -DLANGUAGE_EN_US PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-ZA")
        set(I18N_DEFS -DLANGUAGE_EN_ZA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "en-ZW")
        set(I18N_DEFS -DLANGUAGE_EN_ZW PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "eo-AA")
        set(I18N_DEFS -DLANGUAGE_EO_AA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-AR")
        set(I18N_DEFS -DLANGUAGE_ES_AR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-BO")
        set(I18N_DEFS -DLANGUAGE_ES_BO PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-CL")
        set(I18N_DEFS -DLANGUAGE_ES_CL PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-CO")
        set(I18N_DEFS -DLANGUAGE_ES_CO PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-CR")
        set(I18N_DEFS -DLANGUAGE_ES_CR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-DO")
        set(I18N_DEFS -DLANGUAGE_ES_DO PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-EC")
        set(I18N_DEFS -DLANGUAGE_ES_EC PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-ES")
        set(I18N_DEFS -DLANGUAGE_ES_ES PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-GT")
        set(I18N_DEFS -DLANGUAGE_ES_GT PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-HN")
        set(I18N_DEFS -DLANGUAGE_ES_HN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-MX")
        set(I18N_DEFS -DLANGUAGE_ES_MX PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-NI")
        set(I18N_DEFS -DLANGUAGE_ES_NI PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-PA")
        set(I18N_DEFS -DLANGUAGE_ES_PA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-PE")
        set(I18N_DEFS -DLANGUAGE_ES_PE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-PR")
        set(I18N_DEFS -DLANGUAGE_ES_PR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-PY")
        set(I18N_DEFS -DLANGUAGE_ES_PY PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-SV")
        set(I18N_DEFS -DLANGUAGE_ES_SV PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-UY")
        set(I18N_DEFS -DLANGUAGE_ES_UY PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "es-VE")
        set(I18N_DEFS -DLANGUAGE_ES_VE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "et-EE")
        set(I18N_DEFS -DLANGUAGE_ET_EE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "eu-ES")
        set(I18N_DEFS -DLANGUAGE_EU_ES PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fa-IR")
        set(I18N_DEFS -DLANGUAGE_FA_IR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fi-FI")
        set(I18N_DEFS -DLANGUAGE_FI_FI PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fo-FO")
        set(I18N_DEFS -DLANGUAGE_FO_FO PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fr-BE")
        set(I18N_DEFS -DLANGUAGE_FR_BE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fr-CA")
        set(I18N_DEFS -DLANGUAGE_FR_CA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fr-CH")
        set(I18N_DEFS -DLANGUAGE_FR_CH PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fr-FR")
        set(I18N_DEFS -DLANGUAGE_FR_FR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fr-LU")
        set(I18N_DEFS -DLANGUAGE_FR_LU PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "fr-MC")
        set(I18N_DEFS -DLANGUAGE_FR_MC PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "gl-ES")
        set(I18N_DEFS -DLANGUAGE_GL_ES PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "gu-IN")
        set(I18N_DEFS -DLANGUAGE_GU_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "he-IL")
        set(I18N_DEFS -DLANGUAGE_HE_IL PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "hi-IN")
        set(I18N_DEFS -DLANGUAGE_HI_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "hr-HR")
        set(I18N_DEFS -DLANGUAGE_HR_HR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "hu-HU")
        set(I18N_DEFS -DLANGUAGE_HU_HU PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "hy-AM")
        set(I18N_DEFS -DLANGUAGE_HY_AM PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "id-ID")
        set(I18N_DEFS -DLANGUAGE_ID_ID PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "is-IS")
        set(I18N_DEFS -DLANGUAGE_IS_IS PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "it-CH")
        set(I18N_DEFS -DLANGUAGE_IT_CH PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "it-IT")
        set(I18N_DEFS -DLANGUAGE_IT_IT PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ja-JP")
        set(I18N_DEFS -DLANGUAGE_JA_JP PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ka-GE")
        set(I18N_DEFS -DLANGUAGE_KA_GE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "kk-KZ")
        set(I18N_DEFS -DLANGUAGE_KK_KZ PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "kn-IN")
        set(I18N_DEFS -DLANGUAGE_KN_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "kok-IN")
        set(I18N_DEFS -DLANGUAGE_KOK_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ko-KR")
        set(I18N_DEFS -DLANGUAGE_KO_KR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ky-KG")
        set(I18N_DEFS -DLANGUAGE_KY_KG PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "lt-LT")
        set(I18N_DEFS -DLANGUAGE_LT_LT PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "lv-LV")
        set(I18N_DEFS -DLANGUAGE_LV_LV PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "mk-MK")
        set(I18N_DEFS -DLANGUAGE_MK_MK PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "mn-MN")
        set(I18N_DEFS -DLANGUAGE_MN_MN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "mr-IN")
        set(I18N_DEFS -DLANGUAGE_MR_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ms-BN")
        set(I18N_DEFS -DLANGUAGE_MS_BN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ms-MY")
        set(I18N_DEFS -DLANGUAGE_MS_MY PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "my-MM")
        set(I18N_DEFS -DLANGUAGE_MY_MM PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "nl-BE")
        set(I18N_DEFS -DLANGUAGE_NL_BE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "nl-NL")
        set(I18N_DEFS -DLANGUAGE_NL_NL PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "nn-NO")
        set(I18N_DEFS -DLANGUAGE_NN_NO PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "pa-IN")
        set(I18N_DEFS -DLANGUAGE_PA_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "pl-PL")
        set(I18N_DEFS -DLANGUAGE_PL_PL PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "pt-BR")
        set(I18N_DEFS -DLANGUAGE_PT_BR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "pt-PT")
        set(I18N_DEFS -DLANGUAGE_PT_PT PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "rm-CH")
        set(I18N_DEFS -DLANGUAGE_RM_CH PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ro-RO")
        set(I18N_DEFS -DLANGUAGE_RO_RO PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ru-RU")
        set(I18N_DEFS -DLANGUAGE_RU_RU PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "sa-IN")
        set(I18N_DEFS -DLANGUAGE_SA_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "sk-SK")
        set(I18N_DEFS -DLANGUAGE_SK_SK PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "sl-SI")
        set(I18N_DEFS -DLANGUAGE_SL_SI PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "sq-AL")
        set(I18N_DEFS -DLANGUAGE_SQ_AL PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "sr-SP")
        set(I18N_DEFS -DLANGUAGE_SR_SP PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "sv-FI")
        set(I18N_DEFS -DLANGUAGE_SV_FI PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "sv-SE")
        set(I18N_DEFS -DLANGUAGE_SV_SE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "sw-KE")
        set(I18N_DEFS -DLANGUAGE_SW_KE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "syr-SY")
        set(I18N_DEFS -DLANGUAGE_SYR_SY PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ta-IN")
        set(I18N_DEFS -DLANGUAGE_TA_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "te-IN")
        set(I18N_DEFS -DLANGUAGE_TE_IN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "th-TH")
        set(I18N_DEFS -DLANGUAGE_TH_TH PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "tr-TR")
        set(I18N_DEFS -DLANGUAGE_TR_TR PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "tt-TA")
        set(I18N_DEFS -DLANGUAGE_TT_TA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "uk-UA")
        set(I18N_DEFS -DLANGUAGE_UK_UA PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "ur-PK")
        set(I18N_DEFS -DLANGUAGE_UR_PK PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "uz-UZ")
        set(I18N_DEFS -DLANGUAGE_UZ_UZ PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "vi-VN")
        set(I18N_DEFS -DLANGUAGE_VI_VN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "wa-BE")
        set(I18N_DEFS -DLANGUAGE_WA_BE PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "zh-CN")
        set(I18N_DEFS -DLANGUAGE_ZH_CN PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "zh-HK")
        set(I18N_DEFS -DLANGUAGE_ZH_HK PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "zh-MO")
        set(I18N_DEFS -DLANGUAGE_ZH_MO PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "zh-SG")
        set(I18N_DEFS -DLANGUAGE_ZH_SG PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "zh-TW")
        set(I18N_DEFS -DLANGUAGE_ZH_TW PARENT_SCOPE)
    elseif(I18N_LANG STREQUAL "all")
        set(I18N_DEFS "-DLANGUAGE_AF_ZA -DLANGUAGE_AR_AE -DLANGUAGE_AR_BH -DLANGUAGE_AR_DZ -DLANGUAGE_AR_EG -DLANGUAGE_AR_IQ -DLANGUAGE_AR_JO -DLANGUAGE_AR_KW -DLANGUAGE_AR_LB -DLANGUAGE_AR_LY -DLANGUAGE_AR_MA -DLANGUAGE_AR_OM -DLANGUAGE_AR_QA -DLANGUAGE_AR_SA -DLANGUAGE_AR_SY -DLANGUAGE_AR_TN -DLANGUAGE_AR_YE -DLANGUAGE_AZ_AZ -DLANGUAGE_BE_BY -DLANGUAGE_BG_BG -DLANGUAGE_CA_ES -DLANGUAGE_CS_CZ -DLANGUAGE_DA_DK -DLANGUAGE_DE_AT -DLANGUAGE_DE_CH -DLANGUAGE_DE_DE -DLANGUAGE_DE_LI -DLANGUAGE_DE_LU -DLANGUAGE_DV_MV -DLANGUAGE_EL_GR -DLANGUAGE_EN_AU -DLANGUAGE_EN_BZ -DLANGUAGE_EN_CA -DLANGUAGE_EN_CB -DLANGUAGE_EN_GB -DLANGUAGE_EN_IE -DLANGUAGE_EN_JM -DLANGUAGE_EN_NZ -DLANGUAGE_EN_PH -DLANGUAGE_EN_TT -DLANGUAGE_EN_US -DLANGUAGE_EN_ZA -DLANGUAGE_EN_ZW -DLANGUAGE_EO_AA -DLANGUAGE_ES_AR -DLANGUAGE_ES_BO -DLANGUAGE_ES_CL -DLANGUAGE_ES_CO -DLANGUAGE_ES_CR -DLANGUAGE_ES_DO -DLANGUAGE_ES_EC -DLANGUAGE_ES_ES -DLANGUAGE_ES_GT -DLANGUAGE_ES_HN -DLANGUAGE_ES_MX -DLANGUAGE_ES_NI -DLANGUAGE_ES_PA -DLANGUAGE_ES_PE -DLANGUAGE_ES_PR -DLANGUAGE_ES_PY -DLANGUAGE_ES_SV -DLANGUAGE_ES_UY -DLANGUAGE_ES_VE -DLANGUAGE_ET_EE -DLANGUAGE_EU_ES -DLANGUAGE_FA_IR -DLANGUAGE_FI_FI -DLANGUAGE_FO_FO -DLANGUAGE_FR_BE -DLANGUAGE_FR_CA -DLANGUAGE_FR_CH -DLANGUAGE_FR_FR -DLANGUAGE_FR_LU -DLANGUAGE_FR_MC -DLANGUAGE_GL_ES -DLANGUAGE_GU_IN -DLANGUAGE_HE_IL -DLANGUAGE_HI_IN -DLANGUAGE_HR_HR -DLANGUAGE_HU_HU -DLANGUAGE_HY_AM -DLANGUAGE_ID_ID -DLANGUAGE_IS_IS -DLANGUAGE_IT_CH -DLANGUAGE_IT_IT -DLANGUAGE_JA_JP -DLANGUAGE_KA_GE -DLANGUAGE_KK_KZ -DLANGUAGE_KN_IN -DLANGUAGE_KOK_IN -DLANGUAGE_KO_KR -DLANGUAGE_KY_KG -DLANGUAGE_LT_LT -DLANGUAGE_LV_LV -DLANGUAGE_MK_MK -DLANGUAGE_MN_MN -DLANGUAGE_MR_IN -DLANGUAGE_MS_BN -DLANGUAGE_MS_MY -DLANGUAGE_MY_MM -DLANGUAGE_NL_BE -DLANGUAGE_NL_NL -DLANGUAGE_NN_NO -DLANGUAGE_PA_IN -DLANGUAGE_PL_PL -DLANGUAGE_PT_BR -DLANGUAGE_PT_PT -DLANGUAGE_RM_CH -DLANGUAGE_RO_RO -DLANGUAGE_RU_RU -DLANGUAGE_SA_IN -DLANGUAGE_SK_SK -DLANGUAGE_SL_SI -DLANGUAGE_SQ_AL -DLANGUAGE_SR_SP -DLANGUAGE_SV_FI -DLANGUAGE_SV_SE -DLANGUAGE_SW_KE -DLANGUAGE_SYR_SY -DLANGUAGE_TA_IN -DLANGUAGE_TE_IN -DLANGUAGE_TH_TH -DLANGUAGE_TR_TR -DLANGUAGE_TT_TA -DLANGUAGE_UK_UA -DLANGUAGE_UR_PK -DLANGUAGE_UZ_UZ -DLANGUAGE_VI_VN -DLANGUAGE_WA_BE -DLANGUAGE_ZH_CN -DLANGUAGE_ZH_HK -DLANGUAGE_ZH_MO -DLANGUAGE_ZH_SG -DLANGUAGE_ZH_TW" PARENT_SCOPE)
    else()
        message(SEND_ERROR "${I18N_LANG} doesn't exist in our list. Please select a correct localization.")
    endif()

    message("-- Selected localization: ${I18N_LANG}")

endfunction()
