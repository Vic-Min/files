#include <vsg/all.h>

#ifdef vsgXchange_FOUND
#    include <vsgXchange/all.h>
#endif


static vsg::dsphere bound(const vsg::ref_ptr<vsg::Node>& node)
{
    vsg::ComputeBounds computeBounds;
    node->accept(computeBounds);
    vsg::dvec3 center = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
    double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.5;
    return vsg::dsphere{center[0], center[1], center[2], radius};
}


int main(int argc, char** argv)
{
    // set up defaults and read command line arguments to override them
    auto options = vsg::Options::create();
    options->sharedObjects = vsg::SharedObjects::create();
    options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

#ifdef vsgXchange_all
    // add vsgXchange's support for reading and writing 3rd party file formats
    options->add(vsgXchange::all::create());
#endif

    auto builder = vsg::Builder::create();
    builder->options = options;

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = "vsgtransform";

    // set up defaults and read command line arguments to override them
    vsg::CommandLine arguments(&argc, argv);
    windowTraits->debugLayer = arguments.read({"--debug", "-d"});
    windowTraits->apiDumpLayer = arguments.read({"--api", "-a"});

    arguments.read("--screen", windowTraits->screenNum);
    arguments.read("--display", windowTraits->display);
    auto numFrames = arguments.value(-1, "--fn");
    if (arguments.read({"--fullscreen", "--fs"})) windowTraits->fullscreen = true;
    if (arguments.read({"--window", "-w"}, windowTraits->width, windowTraits->height)) { windowTraits->fullscreen = false; }

    // bool useStagingBuffer = arguments.read({"--staging-buffer", "-s"});

    auto window = vsg::Window::create(windowTraits);
    if (!window)
        return 1;

    auto outputFilename = arguments.value<vsg::Path>("", "-o");

    auto scene = vsg::Group::create();

    vsg::GeometryInfo geomInfo;
    geomInfo.cullNode = arguments.read("--cull");

    vsg::StateInfo stateInfo;
    stateInfo.lighting = false;
    stateInfo.two_sided = true;
    stateInfo.blending = true;

    // green quad
    {
        geomInfo.position = vsg::vec3(0.1f, 0.1f, 0.1f);
        geomInfo.color = vsg::vec4{0.0f, 1.0f, 0.0f, 1.0f};
        auto quad = builder->createQuad(geomInfo, stateInfo);

        auto depthSorted = vsg::DepthSorted::create();
        depthSorted->binNumber = 1;
        depthSorted->bound = bound(quad);
        depthSorted->child = quad;

        scene->addChild(depthSorted);
    }

    // blue quad
    {
        geomInfo.position = vsg::vec3(-0.1f, -0.1f, -0.1f);
        geomInfo.color = vsg::vec4{0.0f, 0.0f, 1.0f, 1.0f};
        auto quad = builder->createQuad(geomInfo, stateInfo);

        auto depthSorted = vsg::DepthSorted::create();
        depthSorted->binNumber = 2;
        depthSorted->bound = bound(quad);
        depthSorted->child = quad;

        scene->addChild(depthSorted);
    }

    // write out scene if required
    if (outputFilename)
    {
        vsg::write(scene, outputFilename, options);
        return 0;
    }

    // create the viewer and assign window(s) to it
    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    vsg::ref_ptr<vsg::LookAt> lookAt;

    // update bounds to new scene extent
    auto bounds = vsg::visit<vsg::ComputeBounds>(scene).bounds;
    vsg::dvec3 centre = (bounds.min + bounds.max) * 0.5;
    double radius = vsg::length(bounds.max - bounds.min) * 0.6;

    // set up the camera
    lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, 0.0, radius * 3.5), centre, vsg::dvec3(0.0, 1.0, 0.0));

    double nearFarRatio = 0.001;
    auto perspective = vsg::Perspective::create(30.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), nearFarRatio * radius, radius * 10.0);

    auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    // add the camera and scene graph to View
    auto view = vsg::View::create();
    view->camera = camera;
    view->addChild(scene);

    // add close handler to respond to the close window button and pressing escape
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));

    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    viewer->compile();

    while (viewer->advanceToNextFrame() && (numFrames < 0 || (numFrames--) > 0))
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }

    return 0;
}
