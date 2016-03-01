@ stub BCryptAddContextFunction
@ stub BCryptAddContextFunctionProvider
@ stdcall BCryptCloseAlgorithmProvider(ptr long)
@ stub BCryptConfigureContext
@ stub BCryptConfigureContextFunction
@ stub BCryptCreateContext
@ stdcall BCryptCreateHash(ptr ptr ptr long ptr long long)
@ stub BCryptDecrypt
@ stub BCryptDeleteContext
@ stub BCryptDeriveKey
@ stdcall BCryptDestroyHash(ptr)
@ stub BCryptDestroyKey
@ stub BCryptDestroySecret
@ stub BCryptDuplicateHash
@ stub BCryptDuplicateKey
@ stub BCryptEncrypt
@ stdcall BCryptEnumAlgorithms(long ptr ptr long)
@ stub BCryptEnumContextFunctionProviders
@ stub BCryptEnumContextFunctions
@ stub BCryptEnumContexts
@ stub BCryptEnumProviders
@ stub BCryptEnumRegisteredProviders
@ stub BCryptExportKey
@ stub BCryptFinalizeKeyPair
@ stdcall BCryptFinishHash(ptr ptr long long)
@ stub BCryptFreeBuffer
@ stdcall BCryptGenRandom(ptr ptr long long)
@ stub BCryptGenerateKeyPair
@ stub BCryptGenerateSymmetricKey
@ stdcall BCryptGetFipsAlgorithmMode(ptr)
@ stdcall BCryptGetProperty(ptr wstr ptr long ptr long)
@ stdcall BCryptHashData(ptr ptr long long)
@ stub BCryptImportKey
@ stub BCryptImportKeyPair
@ stdcall BCryptOpenAlgorithmProvider(ptr wstr wstr long)
@ stub BCryptQueryContextConfiguration
@ stub BCryptQueryContextFunctionConfiguration
@ stub BCryptQueryContextFunctionProperty
@ stub BCryptQueryProviderRegistration
@ stub BCryptRegisterConfigChangeNotify
@ stub BCryptRegisterProvider
@ stub BCryptRemoveContextFunction
@ stub BCryptRemoveContextFunctionProvider
@ stub BCryptResolveProviders
@ stub BCryptSecretAgreement
@ stub BCryptSetAuditingInterface
@ stub BCryptSetContextFunctionProperty
@ stub BCryptSetProperty
@ stub BCryptSignHash
@ stub BCryptUnregisterConfigChangeNotify
@ stub BCryptUnregisterProvider
@ stub BCryptVerifySignature
@ stub GetAsymmetricEncryptionInterface
@ stub GetCipherInterface
@ stub GetHashInterface
@ stub GetRngInterface
@ stub GetSecretAgreementInterface
@ stub GetSignatureInterface
