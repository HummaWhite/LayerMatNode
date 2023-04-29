import maya.mel
import maya.cmds as cmds
import mtoa.ui.ae.utils as aeUtils
from mtoa.ui.ae.shaderTemplate import ShaderAETemplate

class AEla_MetalBSDFTemplate(ShaderAETemplate):
    def changeFresnel(self, nodeName):
        aeUtils.arnoldDimControlIfTrue(nodeName, 'ior', 'schlick_f')
        aeUtils.arnoldDimControlIfTrue(nodeName, 'k', 'schlick_f')

    def setup(self):
        self.addSwatch()
        self.beginScrollLayout()
        self.addCustom('message', 'AEshaderTypeNew', 'AEshaderTypeReplace')

        self.addControl('albedo', label='Albedo')
        self.addControl('ior', label='Index of Refraction (Real)')
        self.addControl('k', label='Index of Refraction (Img)')
        self.addControl('roughness', label='Roughness')
        self.addControl('schlick_f', label='Use Schlick Fresnel', changeCommand=self.changeFresnel)

        self.suppress('normalCamera')

        maya.mel.eval('AEdependNodeTemplate '+self.nodeName)
        self.addExtraControls()
        self.endScrollLayout()
