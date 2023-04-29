import maya.mel
from mtoa.ui.ae.shaderTemplate import ShaderAETemplate

class AEla_MetalBSDFTemplate(ShaderAETemplate):
    def setup(self):
        self.addSwatch()
        self.beginScrollLayout()
        self.addCustom('message', 'AEshaderTypeNew', 'AEshaderTypeReplace')

        self.addControl('albedo', label='Albedo')
        self.addControl('ior', label='Index of Refraction (Real)')
        self.addControl('k', label='Index of Refraction (Img)')
        self.addControl('roughness', label='Roughness')

        maya.mel.eval('AEdependNodeTemplate '+self.nodeName)
        self.addExtraControls()
        self.endScrollLayout()
