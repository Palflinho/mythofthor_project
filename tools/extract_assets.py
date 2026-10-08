#!/usr/bin/env python3
"""
Myth of Thor - Asset Extraction & Pre-processing Utility
-------------------------------------------------------
Este script auxilia na verificação e organização dos arquivos de assets
extraídos do arquivo `data.grf` do Ragnarok Online para a Unreal Engine 5.

Uso:
    python tools/extract_assets.py --help
"""

import os
import sys
import argparse

# Mapas essenciais para o Vertical Slice (Prontera e Treinamento)
TARGET_MAPS = [
    "prontera.rsw",    # Cidade de Prontera (Geometria 3D + Objetos)
    "prontera.gnd",    # Terreno e malha de chão
    "new_1-1.rsw",     # Campo de Treinamento clássico (Castelo)
    "new_1-1.gnd",     # Chão do Campo de Treinamento
    "prt_fild08.rsw",  # Campo Sul de Prontera (Porings, Fabres, Lunatics)
    "prt_fild08.gnd"   # Chão do Campo Sul
]

# Sprites essenciais para o Vertical Slice
TARGET_SPRITES = [
    # Personagens
    "data/sprite/인간족/몸통/남/초보자_남.spr", # Aprendiz Masculino
    "data/sprite/인간족/몸통/남/초보자_남.act",
    "data/sprite/인간족/몸통/여/초보자_여.spr", # Aprendiz Feminino
    "data/sprite/인간족/몸통/여/초보자_여.act",

    # Monstros Iniciais
    "data/sprite/몬스터/poring.spr",
    "data/sprite/몬스터/poring.act",
    "data/sprite/몬스터/lunatic.spr",
    "data/sprite/몬스터/lunatic.act",
    "data/sprite/몬스터/fabre.spr",
    "data/sprite/몬스터/fabre.act",
    "data/sprite/몬스터/pupa.spr",
    "data/sprite/몬스터/pupa.act"
]


def check_grf_file(grf_path: str):
    if not os.path.isfile(grf_path):
        print(f"[ERRO] Arquivo data.grf não encontrado em: {grf_path}")
        print("Por favor, aponte para um arquivo data.grf válido usando --grf <caminho>.")
        return False

    size_mb = os.path.getsize(grf_path) / (1024 * 1024)
    print(f"[OK] data.grf localizado com sucesso! Tamanho: {size_mb:.1f} MB")
    return True


def show_extraction_guide():
    print("=" * 60)
    print("GUIA DE EXTRAÇÃO DE ASSETS PARA O VERTICAL SLICE (PRONTERA)")
    print("=" * 60)
    print("Mapas prioritários a exportar para .FBX ou .OBJ usando o GRF Editor:")
    for m in TARGET_MAPS:
        print(f"  • {m}")
    print("\nSprites prioritários para conversão em Spritesheets PNG:")
    for s in TARGET_SPRITES:
        print(f"  • {s}")
    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(description="Myth of Thor Asset Extraction Tool")
    parser.add_argument("--grf", default="data.grf", help="Caminho para o arquivo data.grf")
    parser.add_argument("--list", action="store_true", help="Lista todos os assets requeridos para o Vertical Slice")
    args = parser.parse_args()

    show_extraction_guide()

    if args.grf:
        check_grf_file(args.grf)


if __name__ == "__main__":
    main()
