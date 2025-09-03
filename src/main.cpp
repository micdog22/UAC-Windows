
// uacctl - Ajuste centralizado do UAC (Windows)
// Autor: MicDog (Michael Douglas)
// Licença: MIT
// Compilar: CMake + MSVC/MinGW. Requer advapi32.
//
// Este utilitário NÃO contorna o UAC nem executa elevação por conta própria.
// Ele apenas grava as chaves oficiais de política do Windows quando
// executado com privilégios de administrador (recomendado rodar como "Local System"
// via PDQ Deploy/SCCM/GPO Startup Script).

#include <windows.h>
#include <sddl.h>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <cwchar>

constexpr const wchar_t* REG_PATH = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";

struct Argv {
    bool modeNeverNotify = false;
    bool modeFullDisable = false;
    bool modeRollback    = false;
    bool modeStatus      = false;
};

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&NtAuthority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0,0,0,0,0,0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

LONG SetDword(const std::wstring& name, DWORD value) {
    HKEY hKey;
    LONG s = RegCreateKeyExW(HKEY_LOCAL_MACHINE, REG_PATH, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (s != ERROR_SUCCESS) return s;
    s = RegSetValueExW(hKey, name.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));
    RegCloseKey(hKey);
    return s;
}

std::optional<DWORD> GetDword(const std::wstring& name) {
    HKEY hKey;
    DWORD data = 0, type = 0, size = sizeof(DWORD);
    LONG s = RegOpenKeyExW(HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_QUERY_VALUE, &hKey);
    if (s != ERROR_SUCCESS) return std::nullopt;
    s = RegQueryValueExW(hKey, name.c_str(), 0, &type, reinterpret_cast<LPBYTE>(&data), &size);
    RegCloseKey(hKey);
    if (s == ERROR_SUCCESS && type == REG_DWORD) return data;
    return std::nullopt;
}

bool ApplyValues(const std::vector<std::pair<std::wstring, DWORD>>& pairs) {
    for (const auto& kv : pairs) {
        LONG s = SetDword(kv.first, kv.second);
        if (s != ERROR_SUCCESS) {
            std::wcerr << L"[ERRO] Falha ao gravar " << kv.first << L" (code " << s << L")\n";
            return false;
        }
    }
    return true;
}

void PrintStatus() {
    auto print = [](const wchar_t* name){
        auto v = GetDword(name);
        if (v.has_value())
            std::wcout << L" - " << name << L" = " << v.value() << L"\n";
        else
            std::wcout << L" - " << name << L" = <não definido>\n";
    };
    std::wcout << L"[Status atual - " << REG_PATH << L"]\n";
    print(L"EnableLUA");
    print(L"ConsentPromptBehaviorAdmin");
    print(L"ConsentPromptBehaviorUser");
    print(L"PromptOnSecureDesktop");
    print(L"EnableInstallerDetection");

    auto enableLUA = GetDword(L"EnableLUA").value_or(1);
    auto admin = GetDword(L"ConsentPromptBehaviorAdmin").value_or(5);
    auto user  = GetDword(L"ConsentPromptBehaviorUser").value_or(3);
    auto secure= GetDword(L"PromptOnSecureDesktop").value_or(1);
    auto inst  = GetDword(L"EnableInstallerDetection").value_or(1);

    std::wcout << L"\n[Interpretação]\n";
    if (enableLUA == 0) {
        std::wcout << L" UAC: DESATIVADO (requer reinicialização se acabou de alterar).\n";
    } else if (admin == 0 && user == 0 && secure == 0 && inst == 0) {
        std::wcout << L" UAC: ATIVO, nível 'Nunca notificar' (sem prompts).\n";
    } else {
        std::wcout << L" UAC: ATIVO com configuração personalizada/padrão.\n";
    }
}

void PrintHelp() {
    std::wcout << L"uacctl - Ajustar o nível do UAC via console\n\n"
               << L"USO:\n"
               << L"  uacctl --status           Mostra valores atuais\n"
               << L"  uacctl --never-notify     Nível 'Nunca notificar' (UAC ativo, sem prompts)\n"
               << L"  uacctl --full-disable     Desativa totalmente o UAC (EnableLUA=0) *requer reboot*\n"
               << L"  uacctl --rollback         Restaura valores recomendados (prompts padrão)\n\n"
               << L"Observação: é necessário executar como Administrador. Não há como alterar\n"
               << L"o UAC sem privilégios elevados. Para automação, execute como 'Local System'\n"
               << L"(PDQ Deploy/SCCM/GPO Startup Script).\n";
}

Argv Parse(int argc, wchar_t** argv) {
    Argv a;
    for (int i = 1; i < argc; ++i) {
        std::wstring s = argv[i];
        if (s == L"--never-notify") a.modeNeverNotify = true;
        else if (s == L"--full-disable") a.modeFullDisable = true;
        else if (s == L"--rollback") a.modeRollback = true;
        else if (s == L"--status") a.modeStatus = true;
        else {
            std::wcout << L"Argumento desconhecido: " << s << L"\n";
            PrintHelp();
            ExitProcess(2);
        }
    }
    if (!a.modeNeverNotify && !a.modeFullDisable && !a.modeRollback && !a.modeStatus) {
        a.modeStatus = true; // padrão
    }
    return a;
}

int wmain(int argc, wchar_t** argv) {
    std::wcout << L"uacctl - Ajuste do UAC (Windows) | MicDog (Michael Douglas)\n";

    Argv a = Parse(argc, argv);

    if (a.modeStatus) {
        PrintStatus();
        return 0;
    }

    if (!IsRunningAsAdmin()) {
        std::wcerr << L"[ERRO] Este utilitário precisa ser executado como Administrador.\n";
        std::wcerr << L"Dica: distribua via PDQ/SCCM/GPO como 'Local System' para evitar prompts.\n";
        return 5; // acesso negado
    }

    if (a.modeNeverNotify) {
        std::wcout << L"Aplicando 'Nunca notificar'...\n";
        bool ok = ApplyValues({
            {L"ConsentPromptBehaviorAdmin", 0},
            {L"ConsentPromptBehaviorUser",  0},
            {L"PromptOnSecureDesktop",      0},
            {L"EnableInstallerDetection",   0},
            {L"EnableLUA",                  1},
        });
        if (!ok) return 1;
        std::wcout << L"Concluído. Reinicialização não é necessária.\n";
        return 0;
    }

    if (a.modeFullDisable) {
        std::wcout << L"Desativando totalmente o UAC (EnableLUA=0)...\n";
        bool ok = ApplyValues({
            {L"EnableLUA", 0},
        });
        if (!ok) return 1;
        std::wcout << L"Concluído. É necessário reiniciar o Windows para efetivar a mudança.\n";
        return 3010; // código comum para "reboot required"
    }

    if (a.modeRollback) {
        std::wcout << L"Restaurando valores recomendados (prompts padrão)...\n";
        bool ok = ApplyValues({
            {L"EnableLUA",                  1},
            {L"ConsentPromptBehaviorAdmin", 5}, // Prompt para admins (padrão)
            {L"ConsentPromptBehaviorUser",  3}, // Solicitar credenciais
            {L"PromptOnSecureDesktop",      1},
            {L"EnableInstallerDetection",   1},
        });
        if (!ok) return 1;
        std::wcout << L"Concluído.\n";
        return 0;
    }

    PrintHelp();
    return 0;
}
