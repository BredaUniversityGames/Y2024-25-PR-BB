bl_info = {
    "name": "BB scene export",
    "blender": (2, 80, 0),
    "category": "View3D",
}

import bpy
import os
from bpy.types import Operator, Panel
from bpy.props import StringProperty

class BBExportSceneSettings(bpy.types.PropertyGroup):
    export_path: StringProperty(
        name="Export Path",
        description="Folder path to export GLTF files",
        default="",
        maxlen=1024,
        subtype='DIR_PATH'
    )

class BBExportScenePanel(Panel):
    bl_label = "scene_level_export_panel"
    bl_idname = "OBJECT_PT_export_gltf"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Tool'

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        
        # path property
        layout.prop(scene.gltf_export_settings, "export_path")
        
        # export button
        row = layout.row()
        row.scale_y = 2.0
        row.operator("export_scene.collections_gltf", text="EXPORT COLLECTIONS")

class BBSceneExporter(Operator):
    bl_idname = "export_scene.collections_gltf"
    bl_label = "Export all collections into a  folder"
    
    def execute(self, context):
        export_path = context.scene.gltf_export_settings.export_path
        
        if not export_path:
            self.report({'ERROR'}, "Please set an export path first")
            return {'CANCELLED'}
        
        if not os.path.exists(export_path):
            self.report({'ERROR'}, "Export path does not exist")
            return {'CANCELLED'}

        # Store initial state
        initial_active_collection = context.view_layer.active_layer_collection
        initial_visible_collections = {coll: coll.hide_viewport for coll in bpy.data.collections}
        initial_selected_objects = [obj for obj in context.selected_objects]
        initial_active_object = context.active_object

        # Deselect all objects
        bpy.ops.object.select_all(action='DESELECT')
        
        successful_exports = []
        failed_exports = []

       for collection in bpy.data.collections:
            try:
                if collection.hide_viewport:
                    continue

               layer_collection = self.find_layer_collection(context.view_layer.layer_collection, collection.name)
                if not layer_collection:
                    continue

                context.view_layer.active_layer_collection = layer_collection

                # Select only objects from this collection
                for obj in collection.objects:
                    if obj.visible_get():
                        obj.select_set(True)
                        context.view_layer.objects.active = obj

               collection_filepath = os.path.join(export_path, f"{collection.name}.gltf")

                # Export the collection
                bpy.ops.export_scene.gltf(
                    filepath=collection_filepath,
                    use_selection=True,
                    export_format='GLTF_EMBEDDED'
                )

                successful_exports.append(collection.name)

                bpy.ops.object.select_all(action='DESELECT')

            except Exception as e:
                failed_exports.append(f"{collection.name} (Error: {str(e)})")
                continue

        # Restore initial state
        context.view_layer.active_layer_collection = initial_active_collection
        for obj in initial_selected_objects:
            obj.select_set(True)
        if initial_active_object:
            context.view_layer.objects.active = initial_active_object
        for coll, hidden in initial_visible_collections.items():
            coll.hide_viewport = hidden

        # Report results
        msg_lines = []
        if successful_exports:
            msg_lines.append(f"Successfully exported {len(successful_exports)} collections:")
            msg_lines.extend(successful_exports)
        if failed_exports:
            msg_lines.append(f"\nFailed to export {len(failed_exports)} collections:")
            msg_lines.extend(failed_exports)

        def draw_popup(self, context):
            for line in msg_lines:
                self.layout.label(text=line)

        bpy.context.window_manager.popup_menu(draw_popup, 
            title="Export Results", 
            icon='FILE_TICK' if not failed_exports else 'ERROR')

        return {'FINISHED'}

    def find_layer_collection(self, layer_collection, collection_name):
        if layer_collection.collection.name == collection_name:
            return layer_collection
        for child in layer_collection.children:
            found = self.find_layer_collection(child, collection_name)
            if found:
                return found
        return None

classes = (
    BBExportSceneSettings,
    BBExportScenePanel,
    BBSceneExporter,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.gltf_export_settings = bpy.props.PointerProperty(type=ExportGLTFSettings)

def unregister():
    for cls in classes:
        bpy.utils.unregister_class(cls)
    del bpy.types.Scene.gltf_export_settings

if __name__ == "__main__":
    register()