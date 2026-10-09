using UnrealBuildTool;
using System.Collections.Generic;

public class MythOfThorEditorTarget : TargetRules
{
	public MythOfThorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MythOfThor");

		// RATIONALE PARA CONFIGURAÇÕES DE COMPILAÇÃO:
		// 1. bOverrideBuildEnvironment = true:
		//    O ambiente possui o MSVC v14.51.36260 (Visual Studio 18 Preview), versão mais recente
		//    que a toolchain recomendada pelo UE 5.8 (14.50.35717). Esta flag autoriza o UBT a prosseguir.
		// 2. bWarningsAsErrors = false:
		//    A nova toolchain do MSVC emite avisos de depreciação estrita (ex.: C4996) em cabeçalhos
		//    internos do próprio motor UE 5.8. Desativar warnings-as-errors evita que o build aborte
		//    por conta do motor enquanto preserva a compilação limpa do módulo MythOfThor.
		// 3. WindowsPlatform.bWriteSarif = false:
		//    Evita crash no gerador experimental de logs SARIF do MSVC (/experimental:log)
		//    ao processar caminhos do sistema operacional Windows com caracteres acentuados.
		bOverrideBuildEnvironment = true;
		bWarningsAsErrors = false;
		WindowsPlatform.bWriteSarif = false;
	}
}
