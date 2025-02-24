add_subdirectory(AbcExport)
add_subdirectory(AbcImport)
add_subdirectory(gpuCache)
	add_subdirectory(AbcBullet)
add_subdirectory(affectsNode)
add_subdirectory(animCubeNode)
add_subdirectory(animExportUtil)
add_subdirectory(animInfoCmd)
add_subdirectory(anisotropicShader)
add_subdirectory(apiMeshShape)
add_subdirectory(arcLenNode)
add_subdirectory(backfillShader)
add_subdirectory(blindComplexDataCmd)
add_subdirectory(blindDataShader)
add_subdirectory(blindDoubleDataCmd)
add_subdirectory(blindShortDataCmd)
add_subdirectory(brickShader)
add_subdirectory(buildRotationNode)
add_subdirectory(cellShader)
add_subdirectory(cameraMessageCmd)
add_subdirectory(checkerShader)
add_subdirectory(circleNode)
add_subdirectory(cleanPerFaceAssignment)
add_subdirectory(clusterWeightFunction)
add_subdirectory(compositingShader)
add_subdirectory(conditionTest)
add_subdirectory(contrastShader)
add_subdirectory(convertBumpCmd)
add_subdirectory(convertEdgesToFacesCmd)
add_subdirectory(convertVerticesToEdgesCmd)
add_subdirectory(convertVerticesToFacesCmd)
add_subdirectory(customPrimitiveGenerator)
add_subdirectory(closestPointOnCurve)
add_subdirectory(closestPointOnNurbsSurfaceCmd)
add_subdirectory(intersectOnNurbsSurfaceCmd)
add_subdirectory(interpZero)
add_subdirectory(interpPlugins)
add_subdirectory(colorTransformData)
add_subdirectory(customAttrManip)
add_subdirectory(customComponentTagNode)
add_subdirectory(createClipCmd)
add_subdirectory(cvColorNode)
add_subdirectory(cvExpandCmd)
add_subdirectory(cvPosCmd)
add_subdirectory(dagMessageCmd)
add_subdirectory(dagPoseInfoCmd)
add_subdirectory(depthShader)
add_subdirectory(cameraSetSubclass)
add_subdirectory(displacementShader)
add_subdirectory(eventTest)
add_subdirectory(lockEvent) 
add_subdirectory(manipOverride)
add_subdirectory(instanceCallbackCmd)
add_subdirectory(exportJointClusterDataCmd)
add_subdirectory(exportSkinClusterDataCmd)
add_subdirectory(externalDropCallback)
add_subdirectory(filteredAsciiFile)
add_subdirectory(fileTexture)
add_subdirectory(findFileTexturesCmd)
add_subdirectory(findTexturesPerPolygonCmd)
add_subdirectory(flameShader)
add_subdirectory(flipUVCmd)
add_subdirectory(footPrintManip)
add_subdirectory(footPrintNode)
add_subdirectory(footPrintNode_GeometryOverride)
add_subdirectory(footPrintNode_AnimatedMaterial)
add_subdirectory(footPrintNode_SubSceneOverride)
add_subdirectory(rawfootPrintNode)
add_subdirectory(squaresNode_noDepthTest)
add_subdirectory(fragmentDumper)
add_subdirectory(fullLoftNode)
add_subdirectory(gammaShader)
add_subdirectory(geomShader)
add_subdirectory(geometryCacheConverter)
add_subdirectory(geometryReplicator)
add_subdirectory(geometryOverrideExample1)
add_subdirectory(geometryOverrideExample2)
# add_subdirectory(geometryOverrideHighPerformance)
add_subdirectory(getAttrAffectsCmd)
add_subdirectory(hairCollisionSolver)
add_subdirectory(helix2Cmd)
add_subdirectory(helixCmd)
add_subdirectory(helixTool)
add_subdirectory(helloCmd)
add_subdirectory(helloWorldCmd)

	add_subdirectory(hwPhongShader)
	add_subdirectory(vp2BlinnShader)
	add_subdirectory(hwApiTextureTest)
	add_subdirectory(customTextureShader)
	add_subdirectory(customSpriteShader)
add_subdirectory(identityNode)
add_subdirectory(identityGeomFilter)
add_subdirectory(basicBlendShape)
add_subdirectory(basicBlendShapeDeformer)
add_subdirectory(basicMorphNode)
add_subdirectory(basicSkinCluster)

if (UNIX)
	add_subdirectory(idleTest)
endif()

add_subdirectory(interpShader)
add_subdirectory(intersectCmd)
add_subdirectory(closestPointCmd)
add_subdirectory(instancerListCmd)
add_subdirectory(jitterNode)
add_subdirectory(fileIOMsgCmd)

if (NOT APPLE)
	add_subdirectory(fluidInfoCmd)
endif()

add_subdirectory(lambertShader)
add_subdirectory(lassoTool)
add_subdirectory(latticeNoise)
add_subdirectory(lavaShader)
add_subdirectory(lensDistortionCallback)
add_subdirectory(lepTranslator)
add_subdirectory(lightShader)
add_subdirectory(listLightLinksCmd)
add_subdirectory(listPolyHolesCmd)
add_subdirectory(marqueeTool)
add_subdirectory(maTranslator)
add_subdirectory(mixtureShader)
add_subdirectory(motionPathCmd)
add_subdirectory(motionPathNode)
add_subdirectory(motionTraceCmd)
add_subdirectory(moveCurveCVsCmd)
add_subdirectory(moveNumericTool)
add_subdirectory(moveTool)
add_subdirectory(moveManip)
add_subdirectory(multiCurveNode)
add_subdirectory(multiPlugInfoCmd)
add_subdirectory(narrowPolyRenderOverride)
add_subdirectory(nodeIconCmd)
add_subdirectory(nodeInfoCmd)
add_subdirectory(nodeMessageCmd)
add_subdirectory(nodeCreatedCBCmd)
add_subdirectory(noiseShader)
add_subdirectory(offsetNode)
add_subdirectory(onbShader)
	add_subdirectory(ownerEmitter)
add_subdirectory(paintCallback)
	add_subdirectory(particleAttrNode)
add_subdirectory(particleSystemInfoCmd)
add_subdirectory(particlePathsCmd)
add_subdirectory(pfxInfoCmd)
add_subdirectory(phongShader)
add_subdirectory(pickCmd)
add_subdirectory(pluginCallbacks)
add_subdirectory(pointOnMeshInfo)
add_subdirectory(pointOnSubdNode)
add_subdirectory(polyMessageCmd)
add_subdirectory(polyPrimitiveCmd)
add_subdirectory(polyTrgNode)
add_subdirectory(progressWindowCmd)
add_subdirectory(rebalanceTransform)
add_subdirectory(referenceQueryCmd)
add_subdirectory(renderAccessNode)

# Commenting this out of the build,
add_subdirectory(captureViewRenderCmd)
add_subdirectory(renderViewRenderCmd)
add_subdirectory(renderViewRenderRegionCmd)
add_subdirectory(richSelectionTool)
add_subdirectory(sampleCmd)
add_subdirectory(sampleParticles)
add_subdirectory(scanDagCmd)
add_subdirectory(scanDagSyntax)
add_subdirectory(sceneAssembly)
add_subdirectory(shadowMatteShader)
add_subdirectory(shellNode)
add_subdirectory(shiftNode)
	add_subdirectory(simpleEmitter)
add_subdirectory(simpleEvaluator)
add_subdirectory(evaluationPruningEvaluator)
add_subdirectory(constraintEvaluator)
add_subdirectory(simpleImageFile)
	add_subdirectory(simpleFluidEmitter)
add_subdirectory(simpleDeformerNode)
add_subdirectory(simpleLoftNode)
add_subdirectory(simpleNoiseShader)
add_subdirectory(simpleSimulationNode)
add_subdirectory(simpleSkipNode)
add_subdirectory(simplePhysicsEngine)
add_subdirectory(simpleSolverNode)
	add_subdirectory(simpleSpring)
add_subdirectory(sineNode)
add_subdirectory(slopeShader)
add_subdirectory(solidCheckerShader)
add_subdirectory(spiralAnimCurveCmd)
add_subdirectory(splitUVCmd)

add_subdirectory(meshOpCmd)

add_subdirectory(surfaceCreateCmd)
add_subdirectory(surfaceTwistCmd)
	add_subdirectory(sweptEmitter)
add_subdirectory(swissArmyManip)
add_subdirectory(tessellatedQuad)
add_subdirectory(testCameraSetCmd)
add_subdirectory(topologyTrackingNode)
add_subdirectory(torusField)
add_subdirectory(transCircleNode)
add_subdirectory(transformDrawNode)
add_subdirectory(translateCmd)
add_subdirectory(undoRedoMsgCmd)
add_subdirectory(userMsgCmd)
add_subdirectory(viewCaptureCmd)
add_subdirectory(viewCallbackTest)
add_subdirectory(volumeShader)
add_subdirectory(weightListNode)
add_subdirectory(volumeLightCmd)
add_subdirectory(whatisCmd)

add_subdirectory(yTwistNode)
add_subdirectory(zoomCameraCmd)
add_subdirectory(dynExprField)
add_subdirectory(rotateManip)
add_subdirectory(surfaceBumpManip)
add_subdirectory(componentScaleManip)
add_subdirectory(deletedMsgCmd)
add_subdirectory(rockingTransform)
add_subdirectory(customImagePlane)

add_subdirectory(exampleRampAttribute)
add_subdirectory(genericAttributeNode)


add_subdirectory(peltOverlapCmd)

add_subdirectory(skinClusterWeights)

add_subdirectory(viewRenderOverride)
add_subdirectory(viewRenderOverrideMRT)
add_subdirectory(viewImageBlitOverride)
add_subdirectory(viewRenderOverrideShadows)
add_subdirectory(viewRenderOverridePointLightShadows)
add_subdirectory(viewObjectFilter)
add_subdirectory(viewRenderOverrideTargets)
add_subdirectory(viewRenderOverrideFrameCache)
add_subdirectory(viewObjectSetOverride)
add_subdirectory(viewMRenderOverride)
add_subdirectory(viewOverrideSimple)
add_subdirectory(viewRenderOverridePostColor)
add_subdirectory(viewOverrideTrackTexture)

#add_subdirectory(viewportDataServer)

add_subdirectory(MayaPluginForSpreticle)

add_subdirectory(vertexBufferGenerator)
add_subdirectory(vertexBufferMutator)

add_subdirectory(uiDrawManager)

add_subdirectory(blast2Cmd)
add_subdirectory(apiDirectionalLightShape)

if (WIN32)
add_subdirectory(viewDX11DeviceAccess)
endif()


