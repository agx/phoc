/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#pragma once

#include "keybindings.h"

#include <phoc-layer-shell-effects-unstable-v1-protocol.h>
#include "layers.h"
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
  PHOC_LAYER_SHELL_EFFECT_DRAG_FROM_TOP = (ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                           ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                                           ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP),
  PHOC_LAYER_SHELL_EFFECT_DRAG_FROM_BOTTOM = (ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                              ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                                              ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM),
  PHOC_LAYER_SHELL_EFFECT_DRAG_FROM_LEFT = (ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                            ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                            ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT),
  PHOC_LAYER_SHELL_EFFECT_DRAG_FROM_RIGHT = (ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                             ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                             ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT)
} PhocLayerShellEffectDrags;

/**
 * PhocDraggableSurfaceState:
 *
 * Drag states of a draggable surface (e.g. a [type@PhocDraggableLayerSurface].
 * TODO: Reuse when xdg surface become draggable too.
 */
typedef enum {
  PHOC_DRAGGABLE_SURFACE_STATE_NONE,
  PHOC_DRAGGABLE_SURFACE_STATE_PENDING,
  PHOC_DRAGGABLE_SURFACE_STATE_DRAGGING,
  PHOC_DRAGGABLE_SURFACE_STATE_ANIMATING,
  PHOC_DRAGGABLE_SURFACE_STATE_REJECTED,
} PhocDraggableSurfaceState;


#define PHOC_TYPE_LAYER_SHELL_EFFECTS (phoc_layer_shell_effects_get_type ())

G_DECLARE_FINAL_TYPE (PhocLayerShellEffects, phoc_layer_shell_effects, PHOC, LAYER_SHELL_EFFECTS, GObject)

typedef enum _AnimDir {
  ANIM_DIR_IN = 0,
  ANIM_DIR_OUT,
} PhocAnimDir;

typedef struct _PhocDraggableLayerSurfaceParams {
  /* Margin when folded / unfolded */
  int32_t  folded, unfolded;
  /* Height of exclusive area */
  uint32_t exclusive;
  /* When is the sufaced pulled out [0.0, 1.0] */
  double   threshold;

  enum zphoc_draggable_layer_surface_v1_drag_mode drag_mode;
  uint32_t drag_handle;
} PhocDraggableLayerSurfaceParams;

typedef struct PhocDraggableLayerSurface {
  struct wl_resource *resource;
  PhocLayerSurface *layer_surface;
  PhocLayerShellEffects *layer_shell_effects;

  /* Double buffered params set by the client */
  PhocDraggableLayerSurfaceParams current;
  PhocDraggableLayerSurfaceParams pending;

  PhocDraggableSurfaceState state;
  struct {
    /* Margin at gesture start */
    int      start_margin;
    int      draggable;
    /* Threshold until drag is accepted */
    int      pending_accept;
    /* Threshold until drag is rejected */
    int      pending_reject;
    /* Slide in/out animation */
    gulong   anim_id;
    float    anim_t;
    int32_t  anim_start;
    int32_t  anim_end;
    PhocAnimDir anim_dir;
    enum zphoc_draggable_layer_surface_v1_drag_end_state last_state;
  } drag;
  struct wlr_box geo;

  struct wl_listener surface_handle_commit;
  struct wl_listener layer_surface_handle_destroy;
  struct {
    struct wl_signal destroy;
  } events;
} PhocDraggableLayerSurface;


PhocLayerShellEffects *phoc_layer_shell_effects_new (void);
void                   phoc_layer_shell_effects_send_drag_start (PhocLayerShellEffects *self,
                                                                 PhocLayerSurface      *surface);
void                   phoc_layer_shell_effects_send_drag_end   (PhocLayerShellEffects *self,
                                                                 PhocLayerSurface      *surface,
                                                                 int                    state);


PhocDraggableLayerSurface *phoc_layer_shell_effects_get_draggable_layer_surface_from_layer_surface (
  PhocLayerShellEffects *self, PhocLayerSurface *layer_surface);

gboolean                 phoc_draggable_layer_surface_is_draggable (PhocDraggableLayerSurface *self);
PhocDraggableSurfaceState phoc_draggable_layer_surface_drag_start  (PhocDraggableLayerSurface *self,
                                                                    double                     lx,
                                                                    double                     ly);
PhocDraggableSurfaceState phoc_draggable_layer_surface_drag_update (PhocDraggableLayerSurface *self,
                                                                    double                     lx,
                                                                    double                     ly);
void                     phoc_draggable_layer_surface_drag_end    (PhocDraggableLayerSurface  *self,
                                                                   double                      lx,
                                                                   double                      ly);
void                     phoc_draggable_layer_surface_slide       (PhocDraggableLayerSurface  *self,
                                                                   PhocAnimDir             anim_dir);
G_END_DECLS
