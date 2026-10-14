#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** Descricao do item do itemInfo.lua do cliente (Config/item_info.json, tools/gen_item_info.py). */
struct FMythRoItemInfo
{
	FString Name;
	TArray<FString> Desc;
	FString UnidentifiedName;
	TArray<FString> UnidentifiedDesc;
	FString Resource;
	int32 Slots = 0;
};

/**
 * Tabelas do cliente original (iteminfo / skillinfo), extraidas do data.grf para Config/items.json e
 * Config/skills.json (tools/extract_client_tables.py). Carregadas sob demanda, uma unica vez.
 * O servidor manda so IDs; nome, descricao e icone sao responsabilidade do cliente, como no original.
 */
struct MYTHOFTHOR_API FMythRoClientTables
{
	/** Nome exibido do item (ex.: 501 -> "Pocao Vermelha"). Vazio se o ID nao existir. */
	static FString ItemName(int32 ItemID);

	/** Nome, descricao e slots do item como no cliente (itemInfo.lua). Nulo se nao existir. */
	static const FMythRoItemInfo* ItemInfo(int32 ItemID);

	/** Ilustracao grande do item (texture/유저인터페이스/collection/<recurso>.bmp; cartas: cardbmp). */
	static UTexture2D* ItemIllustration(int32 ItemID);

	/** Icone 24x24 do item (texture/유저인터페이스/item/<resource_name>.bmp). */
	/** Item icon: the identified item art (item_info.json resource); bIdentified = false gives the unidentified one (items.json resource). */
	static UTexture2D* ItemIcon(int32 ItemID, bool bIdentified = true);

	/** Nome do recurso do item (sprite no chao: data/sprite/아이템/<nome>.spr; icone: item/<nome>.bmp). */
	static FString ItemResource(int32 ItemID);

	/**
	 * Descricao do item que NUNCA volta vazia: itemInfo do cliente (Config/item_info.json) -> items.json -> catalogo do rAthena
	 * (Config/item_catalog.json: nome em ingles, tipo). Sem nenhuma fonte, uma linha com o ID e um erro no log (uma vez por item).
	 */
	static TArray<FString> ItemDescriptionLines(int32 ItemID, bool bIdentified);

	/** Raridade visual do item (0 comum .. 4 lendario), de Config/item_rarity.json (tools/gen_item_rarity.py). */
	static int32 ItemRarity(int32 ItemID);

	/** Cor da raridade (nome no chao e aura), no estilo Diablo: branco, azul, amarelo, roxo, laranja. */
	static FLinearColor RarityColor(int32 Rarity);

	/** Nome exibido da habilidade (ex.: 5 -> "Golpe Fulminante"). */
	static FString SkillName(int32 SkillID);

	/** Icone da habilidade (texture/유저인터페이스/item/<aegis_name minusculo>.bmp). */
	static UTexture2D* SkillIcon(int32 SkillID);

	/** Linhas de descricao da habilidade (skilldesctable do cliente, com cores ^RRGGBB); nulo se nao ha. */
	static const TArray<FString>* SkillDescription(int32 SkillID);


	/** Nome da imagem do minimapa de um mapa (map/<nome>.bmp): new_1-N usa new_zone0N, como no cliente. */
	static FString MapImageName(const FString& MapName);

	/** Linha da msgstringtable do cliente (Config/messages.json), pelo indice; vazio se nao existir. */
	static FString Message(int32 Index);
};
