// Comandos do chat (docs/ref/18): /w e " cochicho, % grupo, emocoes /eN e Alt+0..9, comandos de barra do mundo
// (repassados ao controlador) e o historico de entrada. O servidor segue decidindo tudo; aqui so se escolhe o pacote.

#include "UI/MythRoHUDWidget.h"

#include "Network/RagnarokNetworkSubsystem.h"
#include "Player/MythPlayerController.h"
#include "Sprites/MythEmotion.h"

#include "Components/EditableTextBox.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"

namespace
{
	/** Apelidos de emocao do cliente (/! /? /love ...) -> numero do /eN. */
	int32 EmotionAlias(const FString& Cmd)
	{
		static const TMap<FString, int32> Map = { { TEXT("!"), 1 }, { TEXT("?"), 2 }, { TEXT("love"), 3 }, { TEXT("heart"), 3 } };
		const int32* N = Map.Find(Cmd);
		return N ? *N : 0;
	}
}

bool UMythRoHUDWidget::HandleChatCommand(const FString& Msg)
{
	URagnarokNetworkSubsystem* Net = GetNet();
	AMythPlayerController* PC = Cast<AMythPlayerController>(GetOwningPlayer());
	const bool bOnline = Net && Net->GetNetworkState() == ERagnarokNetworkState::ConnectedMap;
	auto Offline = [this]() { AddChatLine(TEXT("Sem conexão com o servidor."), FLinearColor(1.f, 0.4f, 0.4f)); };

	// Cochicho: "Nome mensagem  (aspas na frente) ou /w Nome mensagem
	if (Msg.StartsWith(TEXT("\"")) || Msg.StartsWith(TEXT("/w ")) || Msg.StartsWith(TEXT("/W ")) || Msg.StartsWith(TEXT("/whisper ")))
	{
		FString Body = Msg.StartsWith(TEXT("\"")) ? Msg.Mid(1) : Msg.Mid(Msg.Find(TEXT(" ")) + 1);
		Body.TrimStartInline();
		FString Name, Text;
		if (!Body.Split(TEXT(" "), &Name, &Text) || Name.IsEmpty() || Text.IsEmpty())
		{
			AddChatLine(TEXT("Uso: /w Nome mensagem"), FLinearColor(1.f, 0.4f, 0.4f));
			return true;
		}
		if (!bOnline) { Offline(); return true; }
		Net->SendWhisper(Name, Text);
		AddChatLine(FString::Printf(TEXT("(para %s) %s"), *Name, *Text), FLinearColor(1.f, 1.f, 0.4f));
		LastWhisperFrom = Name;
		return true;
	}
	if (Msg.StartsWith(TEXT("/r ")) || Msg.StartsWith(TEXT("/R ")))
	{
		if (LastWhisperFrom.IsEmpty())
		{
			AddChatLine(TEXT("Ninguém cochichou com você ainda."), FLinearColor(1.f, 0.4f, 0.4f));
			return true;
		}
		const FString Text = Msg.Mid(3).TrimStartAndEnd();
		if (!bOnline) { Offline(); return true; }
		Net->SendWhisper(LastWhisperFrom, Text);
		AddChatLine(FString::Printf(TEXT("(para %s) %s"), *LastWhisperFrom, *Text), FLinearColor(1.f, 1.f, 0.4f));
		return true;
	}
	// Canal do grupo (%) e da guilda ($, ainda sem guildas)
	if (Msg.StartsWith(TEXT("%")))
	{
		if (!bOnline) { Offline(); return true; }
		Net->SendPartyMessage(Msg.Mid(1).TrimStart());
		return true;
	}
	if (Msg.StartsWith(TEXT("$")))
	{
		AddChatLine(TEXT("O canal da guilda ainda não existe."), FLinearColor(1.f, 0.4f, 0.4f));
		return true;
	}
	if (!Msg.StartsWith(TEXT("/")))
	{
		return false;
	}
	// /comando argumentos
	FString Cmd = Msg.Mid(1), Args;
	Cmd.Split(TEXT(" "), &Cmd, &Args);
	Args.TrimStartAndEndInline();
	// Emocoes: /e1../e64 e apelidos
	int32 Order = EmotionAlias(Cmd);
	if (Order == 0 && Cmd.Len() >= 2 && (Cmd[0] == TEXT('e') || Cmd[0] == TEXT('E')) && Cmd.Mid(1).IsNumeric())
	{
		Order = FCString::Atoi(*Cmd.Mid(1));
	}
	if (Order > 0)
	{
		const int32 Type = AMythEmotion::TypeFromOrder(Order);
		if (Type >= 0 && bOnline) { Net->SendEmotion(Type); }
		return true;
	}
	// Grupo: /organize "nome", /invite nome, /leave, /expulsar nome
	if (Cmd == TEXT("organize") || Cmd == TEXT("invite") || Cmd == TEXT("leave") || Cmd == TEXT("expel"))
	{
		if (!bOnline) { Offline(); return true; }
		FString Arg = Args;
		Arg.TrimQuotesInline();
		if (Cmd == TEXT("leave"))
		{
			Net->SendLeaveParty();
		}
		else if (Arg.IsEmpty())
		{
			AddChatLine(Cmd == TEXT("organize") ? TEXT("Uso: /organize \"nome do grupo\"") : TEXT("Uso: /invite nome do personagem"), FLinearColor(1.f, 0.4f, 0.4f));
		}
		else if (Cmd == TEXT("organize"))
		{
			Net->SendCreateParty(Arg);
		}
		else if (Cmd == TEXT("invite"))
		{
			Net->SendPartyInvite(Arg);
		}
		else if (const FRoPartyMember* M = Net->GetPartyMembers().FindByPredicate([&Arg](const FRoPartyMember& P) { return P.Name.Equals(Arg, ESearchCase::IgnoreCase); }))
		{
			Net->SendPartyExpel(M->AccountId, M->Name);
		}
		else
		{
			AddChatLine(TEXT("Esse personagem não está no seu grupo."), FLinearColor(1.f, 0.4f, 0.4f));
		}
		return true;
	}
	if (PC && PC->RunSlashCommand(Cmd, Args))
	{
		return true;
	}
	AddChatLine(FString::Printf(TEXT("Comando desconhecido: /%s"), *Cmd), FLinearColor(1.f, 0.4f, 0.4f));
	return true;
}

FReply UMythRoHUDWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Historico do chat: Cima volta nas mensagens, Baixo avanca
	if (ChatInput && ChatInput->HasKeyboardFocus() && ChatHistory.Num() > 0)
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::Up || Key == EKeys::Down)
		{
			const int32 Last = ChatHistory.Num() - 1;
			if (Key == EKeys::Up)
			{
				ChatHistoryPos = ChatHistoryPos == INDEX_NONE ? Last : FMath::Max(0, ChatHistoryPos - 1);
			}
			else if (ChatHistoryPos != INDEX_NONE)
			{
				ChatHistoryPos = ChatHistoryPos >= Last ? INDEX_NONE : ChatHistoryPos + 1;
			}
			ChatInput->SetText(FText::FromString(ChatHistoryPos == INDEX_NONE ? FString() : ChatHistory[ChatHistoryPos]));
			return FReply::Handled();
		}
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
