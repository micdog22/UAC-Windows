# uacctl: Ajuste centralizado do UAC (Windows)

**Autor:** MicDog (Michael Douglas)  
**Licença:** MIT

Ferramenta de console para **padronizar o nível do UAC** em estações Windows.  
Suporta:

- `--status`: mostra valores atuais e interpretação
- `--never-notify`: aplica o nível *Nunca notificar* (UAC ativo, sem prompts)
- `--full-disable`: **desativa completamente** o UAC (`EnableLUA=0`); *requer reinício*
- `--rollback`: restaura valores recomendados (prompts padrão)

> **Aviso**: Este utilitário **não** burla o UAC e **não** eleva privilégios por conta própria.  
> Ele apenas grava chaves de política quando **executado como Administrador**.

---

## Compilação (CMake + MSVC)

```bash
git clone https://github.com/seu-usuario/uacctl.git
cd uacctl
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
# binário: build/Release/uacctl.exe
```

O projeto inclui um **manifest** (`requireAdministrator`) embutido via `res/uacctl.rc`.

---

## Uso

```bash
uacctl --status
uacctl --never-notify
uacctl --full-disable    # exige reboot
uacctl --rollback
```

### O que cada modo faz
- **Nunca notificar**: grava
  - `ConsentPromptBehaviorAdmin=0`
  - `ConsentPromptBehaviorUser=0`
  - `PromptOnSecureDesktop=0`
  - `EnableInstallerDetection=0`
  - `EnableLUA=1`

- **Full disable**: grava
  - `EnableLUA=0` (quebra apps UWP/Loja; **requer reinício**)

- **Rollback (recomendado/padrão)**: grava
  - `EnableLUA=1`
  - `ConsentPromptBehaviorAdmin=5`
  - `ConsentPromptBehaviorUser=3`
  - `PromptOnSecureDesktop=1`
  - `EnableInstallerDetection=1`

---

## Distribuição em massa

- **PDQ Deploy / SCCM / RMM**: execute como **Local System** (evita prompts).  
  Exemplo (PDQ): criar um passo *Command* com `uacctl.exe --never-notify`.

- **GPO (Startup Script)**: copie o `uacctl.exe` para `\SYSVOL\...` e adicione um script de inicialização chamando:
  ```bat
  "%~dp0uacctl.exe" --never-notify
  ```
  Rodará como *Local System* na inicialização.

- **Rollback**:
  ```bat
  uacctl.exe --rollback
  ```

---

## Segurança e impactos

- **Nunca notificar** reduz proteção. Avalie o risco.
- **Desativar totalmente** (EnableLUA=0) reduz ainda mais e **quebra aplicativos UWP**.  
  Use apenas quando for absolutamente necessário.
- Para reverter, use `--rollback` e reinicie caso tenha usado `--full-disable` antes.

---

## Créditos

- MicDog (Michael Douglas): idealização e autoria
- Windows Registry / políticas oficiais do UAC

