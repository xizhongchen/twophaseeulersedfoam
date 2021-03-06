    Info<< "Reading transportProperties\n" << endl;

    IOdictionary transportProperties
    (
        IOobject
        (
            "transportProperties",
            runTime.constant(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    );

    autoPtr<phaseModel> phasea = phaseModel::New
    (
        mesh,
        transportProperties,
        "a"
    );

    autoPtr<phaseModel> phaseb = phaseModel::New
    (
        mesh,
        transportProperties,
        "b"
    );

    volVectorField& Ua = phasea->U();
    surfaceScalarField& phia = phasea->phi();
    const dimensionedScalar& rhoa = phasea->rho();
    const dimensionedScalar& nua = phasea->nu();
    const dimensionedScalar& da = phasea->d();

    volVectorField& Ub = phaseb->U();
    surfaceScalarField& phib = phaseb->phi();
    const dimensionedScalar& rhob = phaseb->rho();
    const dimensionedScalar& nub = phaseb->nu();
    const dimensionedScalar& db = phaseb->d();

    Info<< "Reading field alpha\n" << endl;
    volScalarField alpha
    (
        IOobject
        (
            "alpha",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    );

    volScalarField beta
    (
        IOobject
        (
            "beta",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        scalar(1) - alpha
        //,alpha.boundaryField().types()
    );

    Info<< "Reading field p\n" << endl;
    volScalarField p
    (
        IOobject
        (
            "p",
            runTime.timeName(),
            mesh,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh
    );

    volVectorField U
    (
        IOobject
        (
            "U",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        alpha*Ua + beta*Ub
    );

    dimensionedScalar Cvm
    (
        transportProperties.lookup("Cvm")
    );

    dimensionedScalar Cl
    (
        transportProperties.lookup("Cl")
    );

    dimensionedScalar Ct
    (
        transportProperties.lookup("Ct")
    );

    surfaceScalarField phi
    (
        IOobject
        (
            "phi",
            runTime.timeName(),
            mesh
        ),
        fvc::interpolate(alpha)*phia + fvc::interpolate(beta)*phib
    );

    volScalarField rho
    (
        IOobject
        (
            "rho",
            runTime.timeName(),
            mesh
        ),
        alpha*rhoa + beta*rhob
    );

    #include "createRASTurbulence.H"

    Info<< "Calculating field DDtUa and DDtUb\n" << endl;

    volVectorField DDtUa =
        fvc::ddt(Ua)
      + fvc::div(phia, Ua)
      - fvc::div(phia)*Ua;

    volVectorField DDtUb =
        fvc::ddt(Ub)
      + fvc::div(phib, Ub)
      - fvc::div(phib)*Ub;


    Info<< "Calculating field g.h\n" << endl;
    volScalarField gh("gh", g & mesh.C());

    IOdictionary interfacialProperties
    (
        IOobject
        (
            "interfacialProperties",
            runTime.constant(),
            mesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    );

    autoPtr<dragModel> draga = dragModel::New
    (
        interfacialProperties,
        alpha,
        phasea,
        phaseb
    );

    autoPtr<dragModel> dragb = dragModel::New
    (
        interfacialProperties,
        beta,
        phaseb,
        phasea
    );

    word dragPhase("blended");
    if (interfacialProperties.found("dragPhase"))
    {
        dragPhase = word(interfacialProperties.lookup("dragPhase"));

        bool validDrag =
            dragPhase == "a" || dragPhase == "b" || dragPhase == "blended";

        if (!validDrag)
        {
            FatalErrorIn(args.executable())
                << "invalid dragPhase " << dragPhase
                << exit(FatalError);
        }
    }

    Info << "dragPhase is " << dragPhase << endl;
    kineticTheoryModel kineticTheory
    (
        phasea,
        Ub,
        alpha,
        draga
    );

    surfaceScalarField rUaAf
    (
        IOobject
        (
            "rUaAf",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("zero", dimensionSet(0, 0, 1, 0, 0), 0.0)
    );

    surfaceScalarField ppMagf
    (
        IOobject
        (
            "ppMagf",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("zero", dimensionSet(0, 2, -1, 0, 0), 0.0)
    );

    surfaceScalarField ppMagfUEqn
    (
        IOobject
        (
            "ppMagfUEqn",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("zero", dimensionSet(0, 2, -2, 0, 0), 0.0)
    );
    volScalarField lambdaUa
    (
        IOobject
        (
            "lambdaUa",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("zero", dimensionSet(1, -1, -1, 0, 0), 0.0)
    );
    volScalarField K
    (
        IOobject
        (
            "K",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        draga->K(mag(Ub-Ua))
    );
    volScalarField Tp
    (
        IOobject
        (
            "Tp",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("zero", dimensionSet(0, 0, 1, 0, 0), 0.0)
    );
    volScalarField Tl
    (
        IOobject
        (
            "Tl",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("zero", dimensionSet(0, 0, 1, 0, 0), 0.0)
    );
    volScalarField tmf
    (
        IOobject
        (
            "tmf",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("zero", dimensionSet(0, 0, 0, 0, 0), 0.0)
    );
    volVectorField gradPs
    (
        IOobject
        (
            "gradPs",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
//        dimensionedVector("zero", dimensionSet(1, -2, -2, 0, 0), vector::zero)
        dimensionedVector("zero", dimensionSet(0, 1, -2, 0, 0), vector::zero)
    );

    volVectorField gradPf
    (
        IOobject
        (
            "gradPf",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
//        dimensionedVector("zero", dimensionSet(1, -2, -2, 0, 0), vector::zero)
        dimensionedVector("zero", dimensionSet(0, 1, -2, 0, 0), vector::zero)
    );

    volScalarField ppMagfAlpha
    (
        IOobject
        (
            "ppMagfAlpha",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("zero", dimensionSet(1, -1, -2, 0, 0), 0.0)
    );


    label pRefCell = 0;
    scalar pRefValue = 0.0;
    setRefCell(p, mesh.solutionDict().subDict("PIMPLE"), pRefCell, pRefValue);
