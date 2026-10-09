#!/usr/bin/env python3
"""Create the placeholder material used by projectile trajectory previews."""

from __future__ import annotations

import json
import traceback
from pathlib import Path
from typing import Any

import unreal


ASSET_DIR = "/Game/_Resource/FX/Projectile/Trajectory"
MATERIAL_NAME = "M_ProjectileTrajectoryPreview"
MATERIAL_PATH = f"{ASSET_DIR}/{MATERIAL_NAME}"
OUT_PATH = (
    Path(unreal.Paths.project_saved_dir())
    / "MCP"
    / "projectile_trajectory"
    / "create_preview_assets_result.json"
)


def set_any(obj: Any, names: list[str], value: Any) -> str | None:
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception:
            continue
    return None


def create_or_load_material(result: dict[str, Any]) -> Any:
    if not unreal.EditorAssetLibrary.does_directory_exist(ASSET_DIR):
        unreal.EditorAssetLibrary.make_directory(ASSET_DIR)

    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        material = unreal.EditorAssetLibrary.load_asset(MATERIAL_PATH)
        result["created"] = False
        return material

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MATERIAL_NAME,
        ASSET_DIR,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    result["created"] = bool(material)
    return material


def configure_material(material: Any, result: dict[str, Any]) -> None:
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    result["properties"] = {
        "material_domain": set_any(
            material,
            ["material_domain", "MaterialDomain"],
            unreal.MaterialDomain.MD_SURFACE,
        ),
        "blend_mode": set_any(
            material,
            ["blend_mode", "BlendMode"],
            unreal.BlendMode.BLEND_TRANSLUCENT,
        ),
        "shading_model": set_any(
            material,
            ["shading_model", "ShadingModel"],
            unreal.MaterialShadingModel.MSM_UNLIT,
        ),
        "two_sided": set_any(material, ["two_sided", "TwoSided"], True),
        "disable_depth_test": set_any(
            material,
            ["disable_depth_test", "DisableDepthTest"],
            True,
        ),
    }

    color = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionVectorParameter,
        -600,
        -100,
    )
    color.set_editor_property("parameter_name", "PreviewColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.04, 0.47, 1.0, 0.92))

    strength = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        -600,
        40,
    )
    strength.set_editor_property("parameter_name", "EmissiveStrength")
    strength.set_editor_property("default_value", 0.5)

    emissive_strength = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        -300,
        -80,
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(color, "", emissive_strength, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(strength, "", emissive_strength, "B")

    emissive_boost = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant,
        -300,
        40,
    )
    emissive_boost.set_editor_property("r", 2.0)

    emissive = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionMultiply,
        0,
        -80,
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(emissive_strength, "", emissive, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(emissive_boost, "", emissive, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    )

    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionScalarParameter,
        0,
        100,
    )
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.92)

    unreal.MaterialEditingLibrary.connect_material_property(
        opacity,
        "",
        unreal.MaterialProperty.MP_OPACITY,
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    result["saved"] = bool(unreal.EditorAssetLibrary.save_loaded_asset(material))


def main() -> int:
    result: dict[str, Any] = {
        "success": False,
        "material_path": MATERIAL_PATH,
        "created": False,
        "saved": False,
        "errors": [],
    }
    try:
        material = create_or_load_material(result)
        if not material:
            raise RuntimeError(f"Failed to create or load {MATERIAL_PATH}")
        configure_material(material, result)
        result["success"] = bool(result["saved"])
    except Exception:
        result["errors"].append(traceback.format_exc())

    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("PROJECTILE_TRAJECTORY_PREVIEW_RESULT=" + str(OUT_PATH))
    print(json.dumps(result, indent=2))
    return 0 if result["success"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
