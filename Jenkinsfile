node {
    try {
    stage('Setup') {
        println("Setup stage!")
    }
    stage("Build") {
        println("Build stage!")
        sh 'pwd'
        sh 'ls'
        sh 'hostname'
        sh 'make'
    }
    stage("Test") {
        println("Test Stage!")
        // sh 'cd mit_6828_os_labs && make grade'
    }
    stage("Analyze"){
        println("Analyze Stage!")
    }
    } catch (Exception e) {
        
    }
}
