#pragma once

#include "CoreMinimal.h"

/**
 * CourseGenTags
 *
 * Single source of truth for all PCG contract tag constants used across
 * the CourseGen plugin (CourseGenCore + CourseGenRuntime).
 *
 * These tags define the V0 output contract. Any PCG graph that produces
 * course data must tag its outputs using these constants to be consumable
 * by CourseGenRuntime.
 *
 * Convention:
 *   CourseGen.<Category>.<Role>
 *   Category — the broad domain   (Surface, Hole, Bounds, Obstacle, ...)
 *   Role     — the specific slot  (Green, Tee, Candidate, Patch, ...)
 *
 * V0 contract (required for a playable hole):
 *   SurfaceGreen      — dynamic mesh, the playable surface, pre-cut at all candidate positions
 *   HoleTee           — single point, ball spawn transform
 *   HoleCandidate     — point array, possible cup positions (all pre-cut)
 *   HolePatch         — point array, ISM patch positions covering each candidate cut
 *                       must match HoleCandidate count 1:1 via CandidateIndex attribute
 *
 * V1+ contract (reserved, not yet consumed by CourseGenRuntime):
 *   BoundsPlayable    — OOB detection volume
 *   SurfaceZone       — regions with non-default physics
 *   ObstacleCandidate — runtime-selectable obstacles (same slot pattern as holes)
 *   DressingPoint     — atmospheric placement, no gameplay role
 *   LightingHint      — suggested light positions for the theme
 *   AudioZone         — ambient audio regions
 */

namespace CourseGenTags
{
	// ── V0 — Required ─────────────────────────────────────────────────────────

	/** Dynamic mesh. The playable green surface, pre-cut at all candidate positions. */
	static const FName SurfaceGreen      = FName("Green");

	/** Single point. Ball spawn position and aim orientation. */
	static const FName HoleTee           = FName("Tee");

	/**
	 * Point array. Possible cup positions.
	 * Required attribute: CandidateIndex (int32) — unique per point, used to
	 * pair with HolePatch and drive runtime ISM instance selection.
	 */
	static const FName HoleCandidate     = FName("Cup");

	/**
	 * Point array. ISM patch positions covering each candidate cut.
	 * Required attribute: CandidateIndex (int32) — must match a HoleCandidate index.
	 * Required attribute: PatchMesh (FSoftObjectPath) — static mesh to instance.
	 */
	static const FName HolePatch         = FName("CourseGen.Hole.Patch");

	// ── V1 — Reserved, namespace established ─────────────────────────────────

	/** Volume defining OOB extents for ball-out-of-bounds detection. */
	static const FName BoundsPlayable    = FName("CourseGen.Bounds.Playable");

	/** Point array. Regions with non-default physics (ramp, rough, water lip, etc.) */
	static const FName SurfaceZone       = FName("CourseGen.Surface.Zone");

	/** Point array. Runtime-selectable obstacles. Same slot pattern as HoleCandidate. */
	static const FName ObstacleCandidate = FName("CourseGen.Obstacle.Candidate");

	/** Point array. Atmospheric placement. No gameplay role. */
	static const FName DressingPoint     = FName("CourseGen.Dressing.Point");

	/** Point array. Suggested light positions for the active theme. */
	static const FName LightingHint      = FName("CourseGen.Lighting.Hint");

	/** Volume array. Ambient audio regions. */
	static const FName AudioZone         = FName("CourseGen.Audio.Zone");

	// ── Attribute name constants ──────────────────────────────────────────────
	// Attribute names expected on points in the V0 contract.
	// Centralized here to avoid string literals scattered across consumers.

	/** int32 attribute on HoleCandidate and HolePatch points. Pairs them 1:1. */
	static const FName Attr_CandidateIndex = FName("CandidateIndex");

	/** FSoftObjectPath attribute on HolePatch points. Static mesh to instance. */
	static const FName Attr_PatchMesh      = FName("PatchMesh");
}