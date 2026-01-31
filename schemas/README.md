# JSON Schema Documentation

このディレクトリには、DigitShowDSTアプリケーションで使用されるデータファイルのJSON Schemaが含まれています。

## スキーマファイル

### 1. calibration_factor.schema.json

A/DチャンネルおよびD/Aチャンネルの較正係数を定義するスキーマ。

**用途:**
- A/D: 64チャンネル分の較正係数(a, b, c)を保存
- A/D物理量 = a×V² + b×V + c の関係を定義
- D/A: 8チャンネル分の較正係数(da_cal_a, da_cal_b)を保存
- D/A出力 = da_cal_a×物理量 + da_cal_b の関係を定義

**主要フィールド:**
- `version`: スキーマバージョン (例: "1.0")
- `calibration_data`: A/Dチャンネルの較正データ配列 (最大64チャンネル)
  - `channel`: チャンネル番号 (0-63)
  - `cal_a`: 二次係数
  - `cal_b`: 一次係数
  - `cal_c`: 定数項
  - `amp_pb`: アンプ較正基準点の物理量 (オプション)
  - `amp_po`: アンプ較正オフセット点の物理量 (オプション)
- `da_calibration_data`: D/Aチャンネルの較正データ配列 (最大8チャンネル) **NEW**
  - `channel`: チャンネル番号 (0-7)
  - `da_cal_a`: 線形係数 (物理量から電圧への変換)
  - `da_cal_b`: 定数項 (オフセット電圧)
- `initial_specimen`: 初期供試体データ
  - `height_mm`: 高さ [mm]
  - `area_mm2`: 断面積 [mm²]
  - `weight_g`: 重量 [g]
  - `box_weight_g`: ボックス重量 [g]

**バリデーション:**
- `calibration_data`は0〜64チャンネル (省略可能、省略時はa=0, b=0, c=0)
- `da_calibration_data`は0〜8チャンネル (省略可能、省略時はa=0, b=0)
- チャンネル番号は範囲内 (A/D: 0-63, D/A: 0-7)
- すべての係数は数値型

### 2. specimen_data.schema.json
### 3. control_script.schema.json

制御スクリプト（Control from file）を定義するスキーマ。

**用途:**
- CFNUM/CFPARA[0..17] の配列をJSONで保存・読み込み
- 最大128ステップに対応

**主要フィールド:**
- `version`: スキーマバージョン (例: "1.0")
- `steps`: 制御ステップ配列（最大128要素）
  - `step`: ステップ番号（任意。省略時は配列順がインデックス）
  - `cfnum`: 制御関数番号（Control_DA()のディスパッチ番号）
  - `cfpara`: 長さ18の数値配列（CFPARA[0..17]）
  - `description`: 任意の説明文

**バリデーション:**
- `steps`の要素数は1〜128
- `cfpara`は必ず18要素
- 値の単位・意味は `knowledge/control_specifications.md` を参照


供試体の物理特性と寸法を定義するスキーマ。

**用途:**
- 異なる試験ステージでの供試体データを保存
- 4つのステージ: [present, initial, before consolidation, after consolidation]

**主要フィールド:**

#### 配列データ (各4要素)
- `diameter`: 直径 [mm] (円柱供試体の場合)
- `width`: 幅 [mm] (角柱供試体の場合)
- `depth`: 奥行き [mm] (角柱供試体の場合)
- `height`: 高さ [mm]
- `area`: 断面積 [mm²]
- `volume`: 体積 [mm³]
- `weight`: 重量 [g]
- `vldt1`: 鉛直変位計1の値 [mm]
- `vldt2`: 鉛直変位計2の値 [mm]

#### スカラーデータ
- `gs`: 土粒子の比重 (通常 2.0-3.5)
- `membrane_modulus`: メンブレン弾性係数 [kPa]
- `membrane_thickness`: メンブレン厚さ [mm]
- `rod_area`: 載荷ロッド断面積 [mm²]
- `rod_weight`: 載荷ロッド重量 [g]
- `box_weight`: ボックス重量 [g] (一面せん断試験用)

**バリデーション:**
- すべての配列は正確に4要素
- 寸法や重量は非負値
- 比重は2.0-3.5の範囲内

## スキーマの使用方法

### バリデーション

JSON Schemaバリデータを使用してファイルを検証できます:

```bash
# Node.jsの場合
npm install -g ajv-cli
ajv validate -s schemas/calibration_factor.schema.json -d test_data/sample_calibration.json

# Pythonの場合
pip install jsonschema
python -c "import json, jsonschema; \
  schema = json.load(open('schemas/calibration_factor.schema.json')); \
  data = json.load(open('test_data/sample_calibration.json')); \
  jsonschema.validate(data, schema); \
  print('Valid!')"
```

### エディタ統合

多くのエディタはJSON Schemaをサポートしています:

**VS Code:**
JSONファイルの先頭に以下を追加:
```json
{
  "$schema": "../schemas/calibration_factor.schema.json",
  ...
}
```

または`.vscode/settings.json`に:
```json
{
  "json.schemas": [
    {
      "fileMatch": ["*_calibration.json"],
      "url": "./schemas/calibration_factor.schema.json"
    },
    {
      "fileMatch": ["*_specimen.json"],
      "url": "./schemas/specimen_data.schema.json"
    },
    {
      "fileMatch": ["*_control.json", "*.ctl.json"],
      "url": "./schemas/control_script.schema.json"
    }
  ]
}
```

## 仕様

- JSON Schema Draft 7準拠
- `additionalProperties: false` により未定義フィールドを拒否
- すべての必須フィールドは`required`配列で定義
- 数値範囲や配列長の制約を含む

## バージョニング

スキーマのバージョンは`version`フィールドで管理されます:
- メジャーバージョン: 互換性のない変更
- マイナーバージョン: 後方互換性のある追加

現在のバージョン: **1.0**

## 関連ドキュメント

- [JSON Schema Specification](https://json-schema.org/)
- [JSON Schema Draft 7](http://json-schema.org/draft-07/schema#)
- [Understanding JSON Schema](https://json-schema.org/understanding-json-schema/)
