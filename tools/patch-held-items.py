"""Held poses and vanilla shield texture, loaded from the user's own Minecraft resources."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
render=root/'build/guest/src/client/java/dev/libertycraft/client/render'
p=render/'HostAtlas.java';s=p.read_text(encoding='utf-8')
if 'groveShieldUv' not in s:
    s=s.replace('this.height = this.blockH + this.itemH + CRACK_SIZE;',
                'this.height = this.blockH + this.itemH + 64;')
    s=s.replace('this.copyArrows(resources);','this.copyArrows(resources);\n        this.groveCopyShield(resources);')
    marker='\t/** Atlas rect of crack stage 0-9. */'
    s=s.replace(marker,'''    private void groveCopyShield(ResourceManager resources) {
        var id=Identifier.withDefaultNamespace("textures/entity/shield/shield_base_nopattern.png");
        var resource=resources.getResource(id);
        if(resource.isEmpty())return;
        try(var in=resource.get().open();NativeImage image=NativeImage.read(in)){
            for(int y=0;y<64;y++)for(int x=0;x<64;x++){
                int argb=image.getPixel(x*image.getWidth()/64,y*image.getHeight()/64);
                int o=((blockH+itemH+y)*width+256+x)*4;
                pixels.put(o,(byte)(argb>>16));pixels.put(o+1,(byte)(argb>>8));
                pixels.put(o+2,(byte)argb);pixels.put(o+3,(byte)(argb>>>24));
            }
        }catch(java.io.IOException e){dev.libertycraft.LibertyCraft.LOG.warn("Grove shield texture",e);}
    }
    // Vanilla ShieldModel plate: 12 x 22 x 1, front/back/edge cube UVs.
    float[] groveShieldUv(){
        float x=256f,y=blockH+itemH;
        return new float[]{(x+1)/width,(y+1)/height,(x+13)/width,(y+23)/height,
            (x+14)/width,(y+1)/height,(x+26)/width,(y+23)/height,
            x/width,(y+1)/height,(x+1)/width,(y+23)/height};
    }

'''+marker)
    # Width also accommodates the original shield sheet after the arrow strip.
    s=s.replace('ARROW_X + ARROW_TEXTURES.length * ARROW_W);','Math.max(320, ARROW_X + ARROW_TEXTURES.length * ARROW_W));')
    p.write_text(s,encoding='utf-8')
p=render/'WorldExporter.java';s=p.read_text(encoding='utf-8')
start=s.index('        if (!minecraft.player.getMainHandItem().isEmpty()) {')
end=s.index('\t\taddCracks(level);',start)
s=s[:start]+'''        if (!minecraft.player.getMainHandItem().isEmpty()) {
            var stack=minecraft.player.getMainHandItem();
            boolean using=minecraft.player.isUsingItem()&&minecraft.player.getUseItem()==stack;
            if(stack.is(net.minecraft.world.item.Items.SHIELD)){
                ENTITIES.add(new Link.WorldEntity(103,-1,0,0,0,0,using?1:0,.54f,
                    null,atlas.groveShieldUv(),0));
            }else{
                int before=ENTITIES.size();
                addItem(minecraft,level,minecraft.player,stack,Vec3.ZERO,0);
                if(ENTITIES.size()>before){
                    var held=ENTITIES.removeLast();
                    int kind=held.kind()==Proto.WE_BLOCK?100:101;
                    int grip=stack.is(net.minecraft.world.item.Items.BOW)?20:
                        stack.is(net.minecraft.world.item.Items.CROSSBOW)?23:
                        stack.is(net.minecraft.world.item.Items.TRIDENT)?22:0;
                    ENTITIES.add(new Link.WorldEntity(kind,-1,0,0,0,0,using?1:0,
                        kind==100?.24f:grip==20?.60f:.48f,
                        new float[]{grip,using?Math.min(1,minecraft.player.getTicksUsingItem()/20f):0,0},held.uv(),held.tint()));
                }
            }
        }
'''+s[end:]
p.write_text(s,encoding='utf-8')

# Display templates use the owner's actual vanilla block atlas, never world entities/drops.
p=render/'WorldExporter.java';s=p.read_text(encoding='utf-8')
if 'Grove cargo display templates' not in s:
    s=s.replace('\t\tLink.writeWorldEntities(ENTITIES, selection(minecraft, level));',"""
        // Grove cargo display templates; the host selects them from acknowledged inventory.
        var cargoTable=cubeFaces(minecraft,net.minecraft.world.level.block.Blocks.CRAFTING_TABLE.defaultBlockState());
        var cargoBox=cubeFaces(minecraft,net.minecraft.world.level.block.Blocks.OAK_PLANKS.defaultBlockState());
        if(cargoTable!=null)ENTITIES.add(new Link.WorldEntity(104,1,0,0,0,0,0,.36f,null,cargoTable,0));
        if(cargoBox!=null)ENTITIES.add(new Link.WorldEntity(104,2,0,0,0,0,0,.32f,null,cargoBox,0));
        Link.writeWorldEntities(ENTITIES, selection(minecraft, level));
""")
    p.write_text(s,encoding='utf-8')
