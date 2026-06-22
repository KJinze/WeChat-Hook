option casemap:none

EXTERN g_ilink_wrapper_exports:QWORD

.code

CreateNetworkManagerNoPB PROC
    jmp qword ptr [g_ilink_wrapper_exports + 0 * 8]
CreateNetworkManagerNoPB ENDP

DestroyContextNoPB PROC
    jmp qword ptr [g_ilink_wrapper_exports + 1 * 8]
DestroyContextNoPB ENDP

DestroyIlinkStreamContext PROC
    jmp qword ptr [g_ilink_wrapper_exports + 2 * 8]
DestroyIlinkStreamContext ENDP

DestroyLogManagerNoPB PROC
    jmp qword ptr [g_ilink_wrapper_exports + 3 * 8]
DestroyLogManagerNoPB ENDP

DestroyNetworkManagerNoPB PROC
    jmp qword ptr [g_ilink_wrapper_exports + 4 * 8]
DestroyNetworkManagerNoPB ENDP

GetContextNoPB PROC
    jmp qword ptr [g_ilink_wrapper_exports + 5 * 8]
GetContextNoPB ENDP

GetIlinkStreamContext PROC
    jmp qword ptr [g_ilink_wrapper_exports + 6 * 8]
GetIlinkStreamContext ENDP

GetLogManagerNoPB PROC
    jmp qword ptr [g_ilink_wrapper_exports + 7 * 8]
GetLogManagerNoPB ENDP

END
